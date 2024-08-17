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
		mPosition = { -5, 5, 5 };
		mRotation = glm::vec3(90.0f, 0.0f, 0.0f);

		mFocalPoint = glm::vec3(0.0f);
		mDistance = glm::distance(mPosition, mFocalPoint);

		mYaw = 3.0f * (float)mPI / 4.0f;
		mPitch = mPI / 4.0f;
	}

	void Camera::Focus()
	{
	}

	void Camera::Update(float dt)
	{
		if (Input::IsKeyPressed(GLFW_KEY_LEFT_ALT))
		{
			const glm::vec2& mouse{ Input::GetMouseX(), Input::GetMouseY() };
			glm::vec2 delta = mouse - mInitialMousePosition;
			delta *= dt;
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
		mViewMatrix = glm::translate(glm::mat4(1.0f), mPosition) * glm::toMat4(orientation);
		mViewMatrix = glm::inverse(mViewMatrix);
	}

	std::pair<float, float> Camera::PanSpeed() const
	{
		float x = std::min(mViewportWidth / 1000.0f, 2.4f);
		float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		float y = std::min(mViewportHeight / 1000.0f, 2.4f);
		float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return { xFactor, yFactor };
	}

	float Camera::RotationSpeed() const
	{
		return 0.8f;
	}

	float Camera::ZoomSpeed() const
	{
		float distance = mDistance * 0.2f;
		distance = std::max(distance, 0.0f);
		float speed = distance * distance;
		speed = std::min(speed, 100.0f);
		return speed;
	}

	void Camera::MousePan(const glm::vec2& delta)
	{
		auto [xSpeed, ySpeed] = PanSpeed();
		mFocalPoint += -GetRightDirection() * delta.x * xSpeed * mDistance;
		mFocalPoint += GetUpDirection() * delta.y * ySpeed * mDistance;
	}

	void Camera::MouseRotate(const glm::vec2& delta)
	{
		float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
		mYaw += yawSign * delta.x * RotationSpeed();
		mPitch += delta.y * RotationSpeed();
	}

	void Camera::MouseZoom(float delta)
	{
		mDistance -= delta * ZoomSpeed();
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