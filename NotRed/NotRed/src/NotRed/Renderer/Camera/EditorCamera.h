#pragma once

#include "Camera.h"
#include "NotRed/Events/MouseEvent.h"

namespace NR
{
	class EditorCamera : public Camera
	{
	public:
		EditorCamera() = default;
		EditorCamera(const glm::mat4& projectionMatrix);

		void Focus(const glm::vec3& focusPoint);
		void OnUpdate(float ts);
		void OnEvent(Event& e);

		inline float GetDistance() const { return mDistance; }
		inline void SetDistance(float distance) { mDistance = distance; }

		inline void SetViewportSize(uint32_t width, uint32_t height) { mViewportWidth = width; mViewportHeight = height; }

		const glm::mat4& GetViewMatrix() const { return mView; }
		glm::mat4 GetViewProjection() const { return mProjection * mView; }

		glm::vec3 GetUpDirection();
		glm::vec3 GetRightDirection();
		glm::vec3 GetForwardDirection();
		const glm::vec3& GetPosition() const { return mPosition; }
		glm::quat GetOrientation() const;

		float GetPitch() const { return mPitch; }
		float GetYaw() const { return mYaw; }

	private:
		void UpdateCameraView();

		bool OnMouseScroll(MouseScrolledEvent& e);

		void MousePan(const glm::vec2& delta);
		void MouseRotate(const glm::vec2& delta);
		void MouseZoom(float delta);

		glm::vec3 CalculatePosition();

		std::pair<float, float> PanSpeed() const;
		float RotationSpeed() const;
		float ZoomSpeed() const;

	private:
		glm::mat4 mView;
		glm::vec3 mPosition, mRotation, mFocalPoint;

		bool mPanning, mRotating;
		glm::vec2 mInitialMousePosition;
		glm::vec3 mInitialFocalPoint, mInitialRotation;

		float mDistance;
		float mPitch, mYaw;

		float mMinFocusDistance = 100.0f;

		uint32_t mViewportWidth = 1280, mViewportHeight = 720;
	};

}