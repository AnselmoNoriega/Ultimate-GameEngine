#pragma once

#include <glm/detail/type_quat.hpp>

#include "NotRed/Renderer/Camera.h"
#include "NotRed/Core/Events/KeyEvent.h"
#include "NotRed/Core/Events/MouseEvent.h"

namespace NR
{
	enum class CameraMode
	{
		NONE, FLYCAM, ARCBALL
	};

	class EditorCamera : public Camera
	{
	public:
		EditorCamera() = default;
		EditorCamera(float verticalFOV, float aspectRatio, float nearClip, float farClip);

		void Focus(const glm::vec3& focusPoint);
		void Update(float dt);
		void OnEvent(Event& e);

		bool IsActive() const { return mIsActive; }
		void SetActive(bool active) { mIsActive = active; }

		inline float GetDistance() const { return mDistance; }
		inline void SetDistance(float distance) { mDistance = distance; }

		const glm::vec3& GetFocalPoint() const { return mFocalPoint; }

		void SetViewportSize(uint32_t width, uint32_t height);

		const glm::mat4& GetViewMatrix() const { return mViewMatrix; }
		glm::mat4 GetViewProjection() const { return mProjectionMatrix * mViewMatrix; }

		glm::vec3 GetUpDirection() const;
		glm::vec3 GetRightDirection() const;
		glm::vec3 GetForwardDirection() const;

		const glm::vec3& GetPosition() const { return mPosition; }

		glm::quat GetOrientation() const;

		float GetPitch() const { return mPitch; }
		float GetYaw() const { return mYaw; }
		float& GetCameraSpeed() { return mSpeed; }
		float GetCameraSpeed() const { return mSpeed; }

		float GetVerticalFOV() const { return mVerticalFOV; }
		float GetAspectRatio() const { return mAspectRatio; }
		float GetNearClip() const { return mNearClip; }
		float GetFarClip() const { return mFarClip; }
	private:
		void UpdateCameraView();

		bool OnMouseScroll(MouseScrolledEvent& e);
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnKeyReleased(KeyReleasedEvent& e);

		void MousePan(const glm::vec2& delta);
		void MouseRotate(const glm::vec2& delta);
		void MouseZoom(float delta);

		glm::vec3 CalculatePosition() const;

		std::pair<float, float> PanSpeed() const;
		float RotationSpeed() const;
		float ZoomSpeed() const;
	private:
		glm::mat4 mViewMatrix;
		glm::vec3 mPosition, mWorldRotation, mFocalPoint;

		// Perspective projection params
		float mVerticalFOV, mAspectRatio, mNearClip, mFarClip;

		bool mIsActive = false;
		bool mPanning, mRotating;
		glm::vec2 mInitialMousePosition{};
		glm::vec3 mInitialFocalPoint, mInitialRotation;

		float mDistance;
		float mSpeed{ 3.f };
		float mLastSpeed = 0.f;

		float mPitch, mYaw;
		float mPitchDelta{}, mYawDelta{};
		glm::vec3 mPositionDelta{};
		glm::vec3 mRightDirection{};

		CameraMode mCameraMode{ CameraMode::ARCBALL };

		float mMinFocusDistance = 100.0f;

		uint32_t mViewportWidth = 1280, mViewportHeight = 720;
		friend class EditorLayer;
	};
}