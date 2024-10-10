#include "nrpch.h"
#include "EditorCamera.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/quaternion.hpp>

#define GLmENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "NotRed/Core/Input.h"

#define mPI 3.14159f

namespace NR
{
	EditorCamera::EditorCamera(const glm::mat4& projectionMatrix)
		: Camera(projectionMatrix)
	{
		mRotation = glm::vec3(90.0f, 0.0f, 0.0f);
		mFocalPoint = glm::vec3(0.0f);

		glm::vec3 position = { -5, 5, 5 };
		mDistance = glm::distance(position, mFocalPoint);

		mYaw = 3.0f * (float)mPI / 4.0f;
		mPitch = mPI / 4.0f;

		UpdateCameraView();
	}

	void EditorCamera::CalculateYaw(float degrees)
	{
		//Check bounds with the max heading rate so that we aren't moving too fast
		constexpr float maxYawRate = 0.12f;

		degrees = glm::clamp(degrees, -maxYawRate, maxYawRate);

		//This controls how the heading is changed if the camera is pointed straight up or down
		//The heading delta direction changes
		if (mPitch > glm::radians(90.f) && mPitch < glm::radians(270.f) || mPitch < glm::radians(-90.f) && mPitch > glm::radians(-270.f))
		{
			mYaw -= degrees;
		}
		else
		{
			mYaw += degrees;
		}

		//Check bounds for the camera heading
		if (mYaw > glm::radians(360.f))
		{
			mYaw -= glm::radians(360.f);
		}
		else if (mYaw < glm::radians(-360.f))
		{
			mYaw += glm::radians(360.f);
		}
	}

	void EditorCamera::CalculatePitch(float degrees)
	{
		//Check bounds with the max pitch rate so that we aren't moving too fast
		constexpr float maxPitchRate = 0.12f;

		degrees = glm::clamp(degrees, -maxPitchRate, maxPitchRate);
		mPitch += degrees;

		//Check bounds for the camera pitch
		if (mPitch > glm::radians(360.f))
		{
			mPitch -= glm::radians(360.f);
		}
		else if (mPitch < -glm::radians(360.f))
		{
			mPitch += glm::radians(360.f);
		}
	}

	void EditorCamera::UpdateCameraView()
	{
		if (mCameraMode == CameraMode::FLYCAM)
		{
			mRotation = glm::normalize(mFocalPoint - mPosition);
			mRightDirection = glm::cross(mRotation, glm::vec3{ 0.f, 1.f, 0.f });
			mRotation = glm::rotate(glm::normalize(glm::cross(glm::angleAxis(mPitch, mRightDirection),
				glm::angleAxis(mYaw, glm::vec3{ 0.f, 1.f, 0.f }))), mRotation);

			mPosition += mCameraPositionDelta;

			//damping for smooth camera
			mYaw *= 0.5f;
			mPitch *= 0.5f;
			mCameraPositionDelta *= 0.8f;
			mFocalPoint = mPosition + mRotation;
			mViewMatrix = glm::lookAt(mPosition, mFocalPoint, glm::vec3{ 0.f, 1.f, 0.f });
		}
		else
		{
			mPosition = CalculatePosition();
			glm::quat orientation = GetOrientation();
			mRotation = glm::eulerAngles(orientation) * (180.0f / (float)mPI);
			mViewMatrix = glm::translate(glm::mat4(1.0f), mPosition) * glm::toMat4(orientation);
			mViewMatrix = glm::inverse(mViewMatrix);
		}
	}

	void EditorCamera::Focus(const glm::vec3& focusPoint)
	{
		mFocalPoint = focusPoint;
		if (mDistance > mMinFocusDistance)
		{
			float distance = mDistance - mMinFocusDistance;
			MouseZoom(distance / ZoomSpeed());
			UpdateCameraView();
		}
	}

	std::pair<float, float> EditorCamera::PanSpeed() const
	{
		float x = std::min((float)mViewportWidth / 1000.0f, 2.4f); // max = 2.4f
		float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		float y = std::min((float)mViewportHeight / 1000.0f, 2.4f); // max = 2.4f
		float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return { xFactor, yFactor };
	}

	float EditorCamera::RotationSpeed() const
	{
		return 0.8f;
	}

	float EditorCamera::ZoomSpeed() const
	{
		float distance = mDistance * 0.2f;
		distance = std::max(distance, 0.0f);
		float speed = distance * distance;
		speed = std::min(speed, 100.0f); // max speed = 100
		return speed;
	}

	void EditorCamera::Update(float dt)
	{
		const glm::vec2& mouse{ Input::GetMousePositionX(), Input::GetMousePositionY() };
		glm::vec2 delta = (mouse - mInitialMousePosition) * 0.003f;
		mInitialMousePosition = mouse;

		if (mIsActive && Input::IsKeyPressed(KeyCode::LeftAlt))
		{
			mCameraMode = CameraMode::ARCBALL;

			if (Input::IsMouseButtonPressed(MouseButton::Middle))
			{
				MousePan(delta);
			}
			else if (Input::IsMouseButtonPressed(MouseButton::Left))
			{
				MouseRotate(delta);
			}
			else if (Input::IsMouseButtonPressed(MouseButton::Right))
			{
				MouseZoom(delta.x + delta.y);
			}
		}
		else if (Input::IsMouseButtonPressed(MouseButton::Right))
		{
			mCameraMode = CameraMode::FLYCAM;
			if (Input::IsKeyPressed(KeyCode::Q))
			{
				mCameraPositionDelta -= dt * mSpeed * glm::vec3{ 0.f, 1.f, 0.f };
			}
			if (Input::IsKeyPressed(KeyCode::E))
			{
				mCameraPositionDelta += dt * mSpeed * glm::vec3{ 0.f, 1.f, 0.f };
			}
			if (Input::IsKeyPressed(KeyCode::S))
			{
				mCameraPositionDelta -= dt * mSpeed * mRotation;
			}
			if (Input::IsKeyPressed(KeyCode::W))
			{
				mCameraPositionDelta += dt * mSpeed * mRotation;
			}
			if (Input::IsKeyPressed(KeyCode::A))
			{
				mCameraPositionDelta -= dt * mSpeed * mRightDirection;
			}
			if (Input::IsKeyPressed(KeyCode::D))
			{
				mCameraPositionDelta += dt * mSpeed * mRightDirection;
			}
			CalculateYaw(-delta.x);
			CalculatePitch(-delta.y);
		}

		mInitialMousePosition = mouse;

		UpdateCameraView();
	}

	void EditorCamera::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseScrolledEvent>(NR_BIND_EVENT_FN(EditorCamera::OnMouseScroll));
	}

	bool EditorCamera::OnMouseScroll(MouseScrolledEvent& e)
	{
		float delta = e.GetYOffset() * 0.1f;
		MouseZoom(delta);
		UpdateCameraView();
		return false;
	}

	void EditorCamera::MousePan(const glm::vec2& delta)
	{
		auto [xSpeed, ySpeed] = PanSpeed();
		mFocalPoint += -GetRightDirection() * delta.x * xSpeed * mDistance;
		mFocalPoint += GetUpDirection() * delta.y * ySpeed * mDistance;
	}

	void EditorCamera::MouseRotate(const glm::vec2& delta)
	{
		float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
		mYaw += yawSign * delta.x * RotationSpeed();
		mPitch += delta.y * RotationSpeed();
	}

	void EditorCamera::MouseZoom(float delta)
	{
		mDistance -= delta * ZoomSpeed();
		if (mDistance < 1.0f)
		{
			mFocalPoint += GetForwardDirection();
			mDistance = 1.0f;
		}
	}

	glm::vec3 EditorCamera::GetUpDirection()
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::vec3 EditorCamera::GetRightDirection()
	{
		return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	glm::vec3 EditorCamera::GetForwardDirection()
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
	}

	glm::vec3 EditorCamera::CalculatePosition()
	{
		return mFocalPoint - GetForwardDirection() * mDistance;
	}

	glm::quat EditorCamera::GetOrientation() const
	{
		return glm::quat(glm::vec3(-mPitch, -mYaw, 0.0f));
	}
}