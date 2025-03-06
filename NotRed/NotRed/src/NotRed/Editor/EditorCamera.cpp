#include "nrpch.h"
#include "EditorCamera.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "NotRed/Core/Input.h"

#include "NotRed/Core/Application.h"
#include "NotRed/ImGui/ImGui.h"

#define M_PI 3.14159f

namespace NR
{
	EditorCamera::EditorCamera(float verticalFOV, float aspectRatio, float nearClip, float farClip)
		: Camera(glm::perspective(verticalFOV, aspectRatio, nearClip, farClip)), mVerticalFOV(verticalFOV), mAspectRatio(aspectRatio), mNearClip(nearClip), mFarClip(farClip)
	{
		mFocalPoint = glm::vec3(0.0f);

		glm::vec3 position = { -5, 5, 5 };
		mDistance = glm::distance(position, mFocalPoint);

		mYaw = 3.0f * (float)M_PI / 4.0f;
		mPitch = M_PI / 4.0f;

		mPosition = CalculatePosition();
		const glm::quat orientation = GetOrientation();
		mWorldRotation = glm::eulerAngles(orientation) * (180.0f / (float)M_PI);
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

	void EditorCamera::Update(float dt)
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

				if (Input::IsKeyPressed(KeyCode::Q))
					mPositionDelta -= dt * mSpeed * glm::vec3{ 0.f, yawSign, 0.f };
				if (Input::IsKeyPressed(KeyCode::E))
					mPositionDelta += dt * mSpeed * glm::vec3{ 0.f, yawSign, 0.f };
				if (Input::IsKeyPressed(KeyCode::S))
					mPositionDelta -= dt * mSpeed * mWorldRotation;
				if (Input::IsKeyPressed(KeyCode::W))
					mPositionDelta += dt * mSpeed * mWorldRotation;
				if (Input::IsKeyPressed(KeyCode::A))
					mPositionDelta -= dt * mSpeed * mRightDirection;
				if (Input::IsKeyPressed(KeyCode::D))
					mPositionDelta += dt * mSpeed * mRightDirection;

				constexpr float maxRate{ 0.12f };
				mYawDelta += glm::clamp(yawSign * delta.x * RotationSpeed(), -maxRate, maxRate);
				mPitchDelta += glm::clamp(delta.y * RotationSpeed(), -maxRate, maxRate);

				mRightDirection = glm::cross(mWorldRotation, glm::vec3{ 0.f, yawSign, 0.f });

				mWorldRotation = glm::rotate(glm::normalize(glm::cross(glm::angleAxis(-mPitchDelta, mRightDirection),
					glm::angleAxis(-mYawDelta, glm::vec3{ 0.f, yawSign, 0.f }))), mWorldRotation);
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
					EnableMouse();
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
			mPosition = CalculatePosition();

		UpdateCameraView();
	}

	void EditorCamera::UpdateCameraView()
	{
		const float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;

		// Extra step to handle the problem when the camera direction is the same as the up vector
		const float cosAngle = glm::dot(GetForwardDirection(), GetUpDirection());
		if (cosAngle * yawSign > 0.99f)
			mPitchDelta = 0.f;

		const glm::vec3 lookAt = mPosition + GetForwardDirection();
		mWorldRotation = glm::normalize(mFocalPoint - mPosition);
		mFocalPoint = mPosition + GetForwardDirection() * mDistance;
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
		if (mDistance > mMinFocusDistance)
		{
			const float distance = mDistance - mMinFocusDistance;
			MouseZoom(distance / ZoomSpeed());
			mCameraMode = CameraMode::ARCBALL;
		}
		mPosition = mFocalPoint - GetForwardDirection() * mDistance;
		UpdateCameraView();
	}

	std::pair<float, float> EditorCamera::PanSpeed() const
	{
		const float x = std::min(float(mViewportWidth) / 1000.0f, 2.4f); // max = 2.4f
		const float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		const float y = std::min(float(mViewportHeight) / 1000.0f, 2.4f); // max = 2.4f
		const float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return { xFactor, yFactor };
	}

	float EditorCamera::RotationSpeed() const
	{
		return 0.3f;
	}

	float EditorCamera::ZoomSpeed() const
	{
		float distance = mDistance * 0.2f;
		distance = std::max(distance, 0.0f);
		float speed = distance * distance;
		speed = std::min(speed, 100.0f); // max speed = 100
		return speed;
	}

	void EditorCamera::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& e) { return OnMouseScroll(e); });
		dispatcher.Dispatch<KeyReleasedEvent>([this](KeyReleasedEvent& e) { return OnKeyReleased(e); });
		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& e) { return OnKeyPressed(e); });
	}

	bool EditorCamera::OnMouseScroll(MouseScrolledEvent& e)
	{
		if (mIsActive)
		{
			if (Input::IsMouseButtonPressed(MouseButton::Right))
			{
				e.GetYOffset() > 0 ? mSpeed += 0.3f * mSpeed : mSpeed -= 0.3f * mSpeed;
				mSpeed = std::clamp(mSpeed, 0.0005f, 2.f);
			}
			else
			{
				MouseZoom(e.GetYOffset() * 0.1f);
				UpdateCameraView();
			}
		}

		return false;
	}

	bool EditorCamera::OnKeyPressed(KeyPressedEvent& e)
	{
		if (mLastSpeed == 0.0f)
		{
			if (e.GetKeyCode() == KeyCode::LeftShift)
			{
				mLastSpeed = mSpeed;
				mSpeed *= 2.0f - glm::log(mSpeed);
			}
			if (e.GetKeyCode() == KeyCode::LeftControl)
			{
				mLastSpeed = mSpeed;
				mSpeed /= 2.0f - glm::log(mSpeed);
			}

			mSpeed = glm::clamp(mSpeed, 0.0005f, 2.0f);
		}
		return true;
	}

	bool EditorCamera::OnKeyReleased(KeyReleasedEvent& e)
	{
		if (e.GetKeyCode() == KeyCode::LeftShift || e.GetKeyCode() == KeyCode::LeftControl)
		{
			if (mLastSpeed != 0.0f)
			{
				mSpeed = mLastSpeed;
				mLastSpeed = 0.0f;
			}
			mSpeed = glm::clamp(mSpeed, 0.0005f, 2.0f);
		}
		return true;
	}

	void EditorCamera::MousePan(const glm::vec2& delta)
	{
		auto [xSpeed, ySpeed] = PanSpeed();
		mFocalPoint += -GetRightDirection() * delta.x * xSpeed * mDistance;
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
		mPosition = mFocalPoint - GetForwardDirection() * mDistance;
		const glm::vec3 forwardDir = GetForwardDirection();
		if (mDistance < 1.0f)
		{
			mFocalPoint += forwardDir;
			mDistance = 1.0f;
		}
		mPositionDelta += delta * ZoomSpeed() * forwardDir;
	}

	void EditorCamera::SetViewportSize(uint32_t width, uint32_t height)
	{
		mViewportWidth = width;
		mViewportHeight = height;

		mAspectRatio = (float)width / (float)height;
		SetProjectionMatrix(glm::perspective(mVerticalFOV, mAspectRatio, mNearClip, mFarClip));
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