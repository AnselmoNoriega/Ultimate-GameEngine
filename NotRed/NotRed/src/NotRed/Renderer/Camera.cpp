#include "nrpch.h"
#include "Camera.h"

#include "NotRed/Core/Input.h"

#include <glfw/glfw3.h>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#define mPI 3.14159f

namespace NR
{
	Camera::Camera(const glm::mat4& projectionMatrix)
		: mProjectionMatrix(projectionMatrix)
	{
		// Sensible defaults
		mPanSpeed = 0.0015f;
		mRotationSpeed = 0.002f;
		mZoomSpeed = 0.2f;

		mPosition = { -100, 100, 100 };
		mRotation = glm::vec3(90.0f, 0.0f, 0.0f);

		mFocalPoint = glm::vec3(0.0f);
		mDistance = glm::distance(mPosition, mFocalPoint);

		mYaw = 3.0f * (float)mPI / 4.0f;
		mPitch = mPI / 4.0f;
	}

	void Camera::Focus()
	{
	}

	void Camera::Update()
	{
		if (Input::IsKeyPressed(GLFW_KEY_LEFT_ALT))
		{
			const glm::vec2& mouse{ Input::GetMouseX(), Input::GetMouseY() };
			glm::vec2 delta = mouse - mInitialMousePosition;
			mInitialMousePosition = mouse;

			if (Input::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_MIDDLE))
			{
				MousePan(delta);
			}
			else if (Input::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
			{
				MouseRotate(delta);
			}
			else if (Input::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
			{
				MouseZoom(delta.y);
			}
		}

		mPosition = CalculatePosition();

		glm::quat orientation = GetOrientation();
		mRotation = glm::eulerAngles(orientation) * (180.0f / (float)mPI);
		mViewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 1)) * glm::toMat4(glm::conjugate(orientation)) * glm::translate(glm::mat4(1.0f), -mPosition);
	}

	void Camera::MousePan(const glm::vec2& delta)
	{
		mFocalPoint += -GetRightDirection() * delta.x * mPanSpeed * mDistance;
		mFocalPoint += GetUpDirection() * delta.y * mPanSpeed * mDistance;
	}

	void Camera::MouseRotate(const glm::vec2& delta)
	{
		float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
		mYaw += yawSign * delta.x * mRotationSpeed;
		mPitch += delta.y * mRotationSpeed;
	}

	void Camera::MouseZoom(float delta)
	{
		mDistance -= delta * mZoomSpeed;
		if (mDistance < 1.0f)
		{
			mFocalPoint += GetForwardDirection();
			mDistance = 1.0f;
		}
	}

	glm::vec3 Camera::GetUpDirection()
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::vec3 Camera::GetRightDirection()
	{
		return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	glm::vec3 Camera::GetForwardDirection()
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
	}

	glm::vec3 Camera::CalculatePosition()
	{
		return mFocalPoint - GetForwardDirection() * mDistance;
	}

	glm::quat Camera::GetOrientation()
	{
		return glm::quat(glm::vec3(-mPitch, -mYaw, 0.0f));
	}
}