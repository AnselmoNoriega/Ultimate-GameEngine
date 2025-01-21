#include "nrpch.h"
#include "EditorCamera.h"

#include <imgui.h>

#include <GLFW/glfw3.h>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "NotRed/Core/Input.h"

#include "NotRed/Core/Application.h"

#include "NotEditor/src/EditorLayer.h"

#define M_PI 3.14159f

namespace NR
{
    EditorCamera::EditorCamera(const glm::mat4& projectionMatrix)
        : Camera(projectionMatrix), mFocalPoint(0.0f)
    {

        const glm::vec3 position = { -5, 5, 5 };
        mDistance = glm::distance(position, mFocalPoint);

        mYaw = 3.0f * (float)M_PI / 4.0f;
        mPitch = M_PI / 4.0f;

        mPosition = CalculatePosition();
        const glm::quat orientation = GetOrientation();
        mRotation = glm::eulerAngles(orientation) * (180.0f / (float)M_PI);
        mViewMatrix = glm::translate(glm::mat4(1.0f), mPosition) * glm::toMat4(orientation);
        mViewMatrix = glm::inverse(mViewMatrix);
    }

    static void DisableMouse()
    {
        Input::SetCursorMode(CursorMode::Locked);
        UI::SetMouseEnabled(false);
    }

    static void EnableMouse()
    {
        Input::SetCursorMode(CursorMode::Normal);
        UI::SetMouseEnabled(true);
    }

    void EditorCamera::Update(const float dt)
    {
        const glm::vec2& mouse{ Input::GetMousePositionX(), Input::GetMousePositionY() };
        const glm::vec2 delta = (mouse - mInitialMousePosition) * 0.002f;

        if (mIsActive)
        {
            if (Input::IsMouseButtonPressed(MouseButton::Right) && !Input::IsKeyPressed(KeyCode::LeftAlt))
            {
                mCameraMode = CameraMode::FLYCAM;
                DisableMouse();
                const float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;

                const float speed = GetCameraSpeed();

                if (Input::IsKeyPressed(KeyCode::Q))
                {
                    mPositionDelta -= dt * speed * glm::vec3{ 0.f, yawSign, 0.f };
                }
                if (Input::IsKeyPressed(KeyCode::E))
                {
                    mPositionDelta += dt * speed * glm::vec3{ 0.f, yawSign, 0.f };
                }
                if (Input::IsKeyPressed(KeyCode::S))
                {
                    mPositionDelta -= dt * speed * mRotation;
                }
                if (Input::IsKeyPressed(KeyCode::W))
                {
                    mPositionDelta += dt * speed * mRotation;
                }
                if (Input::IsKeyPressed(KeyCode::A))
                {
                    mPositionDelta -= dt * speed * mRightDirection;
                }
                if (Input::IsKeyPressed(KeyCode::D))
                {
                    mPositionDelta += dt * speed * mRightDirection;
                }

                constexpr float maxRate{ 0.12f };
                mYawDelta += glm::clamp(yawSign * delta.x * RotationSpeed(), -maxRate, maxRate);
                mPitchDelta += glm::clamp(delta.y * RotationSpeed(), -maxRate, maxRate);

                mRightDirection = glm::cross(mRotation, glm::vec3{ 0.f, yawSign, 0.f });

                mRotation = glm::rotate(
                    glm::normalize(glm::cross(glm::angleAxis(-mPitchDelta, mRightDirection),
                    glm::angleAxis(-mYawDelta, glm::vec3{ 0.f, yawSign, 0.f }))), mRotation);

                const float distance = glm::distance(mFocalPoint, mPosition);
                mFocalPoint = mPosition + GetForwardDirection() * distance;
                mDistance = distance;
            }
            else if (Input::IsKeyPressed(KeyCode::LeftAlt))
            {
                mCameraMode = CameraMode::ARCBALL;

                if (Input::IsMouseButtonPressed(MouseButton::Middle))
                {
                    DisableMouse();
                    MousePan(delta);
                }
                else if (Input::IsMouseButtonPressed(MouseButton::Left))
                {
                    DisableMouse();
                    MouseRotate(delta);
                }
                else if (Input::IsMouseButtonPressed(MouseButton::Right))
                {
                    DisableMouse();
                    MouseZoom(delta.x + delta.y);
                }
                else
                {
                    EnableMouse();
                }
            }
            else
            {
                EnableMouse();
            }
        }
        mInitialMousePosition = mouse;

        mPosition += mPositionDelta;
        mYaw += mYawDelta;
        mPitch += mPitchDelta;

        if (mCameraMode == CameraMode::ARCBALL)
        {
            mPosition = CalculatePosition();
        }
        UpdateCameraView();
    }

    float EditorCamera::GetCameraSpeed() const
    {
        float speed = mNormalSpeed;
        if (Input::IsKeyPressed(KeyCode::LeftControl))
        {
            speed /= 3 - glm::log(mNormalSpeed);
        }
        if (Input::IsKeyPressed(KeyCode::LeftShift))
        {
            speed *= 3 - glm::log(mNormalSpeed);
        }

        return glm::clamp(speed, MIN_SPEED, MAX_SPEED);
    }

    void EditorCamera::UpdateCameraView()
    {
        const float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;

        // Extra step to handle the problem when the camera direction is the same as the up vector
        const float cosAngle = glm::dot(GetForwardDirection(), GetUpDirection());
        if (cosAngle * yawSign > 0.99f)
        {
            mPitchDelta = 0.0f;
        }

        const glm::vec3 lookAt = mPosition + GetForwardDirection();
        mRotation = glm::normalize(lookAt - mPosition);
        mDistance = glm::distance(mPosition, mFocalPoint);
        mViewMatrix = glm::lookAt(mPosition, lookAt, glm::vec3{ 0.f, yawSign, 0.f });

        //damping for smooth camera
        mYawDelta *= 0.6f;
        mPitchDelta *= 0.6f;
        mPositionDelta *= 0.8f;
    }

    void EditorCamera::Focus(const glm::vec3& focusPoint)
    {
        mFocalPoint = focusPoint;
        mCameraMode = CameraMode::FLYCAM;
        if (mDistance > mMinFocusDistance)
        {
            mDistance -= mDistance - mMinFocusDistance;
            mPosition = mFocalPoint - GetForwardDirection() * mDistance;
        }
        mPosition = mFocalPoint - GetForwardDirection() * mDistance;
        UpdateCameraView();
    }

    std::pair<float, float> EditorCamera::PanSpeed() const
    {
        const float x = glm::min(float(mViewportWidth) / 1000.0f, 2.4f);
        const float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

        const float y = glm::min(float(mViewportHeight) / 1000.0f, 2.4f);
        const float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

        return { xFactor, yFactor };
    }

    float EditorCamera::RotationSpeed() const
    {
        return 0.8f;
    }

    float EditorCamera::ZoomSpeed() const
    {
        float distance = mDistance * 0.2f;
        distance = glm::max(distance, 0.0f);
        float speed = distance * distance;
        speed = glm::min(speed, 50.0f); // max speed = 50
        return speed;
    }

    void EditorCamera::OnEvent(Event& event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& e) { return MouseScroll(e); });
    }

    bool EditorCamera::MouseScroll(MouseScrolledEvent& e)
    {
        if (Input::IsMouseButtonPressed(MouseButton::Right))
        {
            mNormalSpeed += e.GetYOffset() * 0.3f * mNormalSpeed;
            mNormalSpeed = std::clamp(mNormalSpeed, MIN_SPEED, MAX_SPEED);
        }
        else
        {
            MouseZoom(e.GetYOffset() * 0.1f);
            UpdateCameraView();
        }

        return true;
    }

    void EditorCamera::MousePan(const glm::vec2& delta)
    {
        auto [xSpeed, ySpeed] = PanSpeed();
        mFocalPoint -= GetRightDirection() * delta.x * xSpeed * mDistance;
        mFocalPoint += GetUpDirection() * delta.y * ySpeed * mDistance;
    }

    void EditorCamera::MouseRotate(const glm::vec2& delta)
    {
        const float yawSign = GetUpDirection().y < 0.0f ? -1.0f : 1.0f;
        mYawDelta += yawSign * delta.x * RotationSpeed();
        mPitchDelta += delta.y * RotationSpeed();
    }

    void EditorCamera::MouseZoom(float delta)
    {
        mDistance -= delta * ZoomSpeed();
        const glm::vec3 forwardDir = GetForwardDirection();
        mPosition = mFocalPoint - forwardDir * mDistance;
        if (mDistance < 1.0f)
        {
            mFocalPoint += forwardDir * mDistance;
            mDistance = 1.0f;
        }
        mPositionDelta += delta * ZoomSpeed() * forwardDir;
    }

    glm::vec3 EditorCamera::GetUpDirection() const
    {
        return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::vec3 EditorCamera::GetRightDirection() const
    {
        return glm::rotate(GetOrientation(), glm::vec3(1.f, 0.f, 0.f));
    }

    glm::vec3 EditorCamera::GetForwardDirection() const
    {
        return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
    }

    glm::vec3 EditorCamera::CalculatePosition() const
    {
        return mFocalPoint - GetForwardDirection() * mDistance + mPositionDelta;
    }

    glm::quat EditorCamera::GetOrientation() const
    {
        return glm::quat(glm::vec3(-mPitch - mPitchDelta, -mYaw - mYawDelta, 0.0f));
    }
}