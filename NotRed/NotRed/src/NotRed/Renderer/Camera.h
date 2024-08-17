#pragma once

#include <glm/glm.hpp>

namespace NR
{
	class Camera
	{
	public:
		Camera(const glm::mat4& projectionMatrix);

		void Focus();
		void Update(float dt);

		inline float GetDistance() const { return mDistance; }
		inline void SetDistance(float distance) { mDistance = distance; }

		inline void SetProjectionMatrix(const glm::mat4& projectionMatrix) { mProjectionMatrix = projectionMatrix; }
		inline void SetViewportSize(uint32_t width, uint32_t height) { mViewportWidth = width; mViewportHeight = height; }

		const glm::mat4& GetProjectionMatrix() const { return mProjectionMatrix; }
		const glm::mat4& GetViewMatrix() const { return mViewMatrix; }

		glm::vec3 GetUpDirection();
		glm::vec3 GetRightDirection();
		glm::vec3 GetForwardDirection();
		const glm::vec3& GetPosition() const { return mPosition; }

		std::pair<float, float> PanSpeed() const;
		float RotationSpeed() const;
		float ZoomSpeed() const;

	private:
		void MousePan(const glm::vec2& delta);
		void MouseRotate(const glm::vec2& delta);
		void MouseZoom(float delta);

		glm::vec3 CalculatePosition();
		glm::quat GetOrientation();

	private:
		glm::mat4 mProjectionMatrix, mViewMatrix;
		glm::vec3 mPosition, mRotation, mFocalPoint;

		uint32_t mViewportWidth = 1280, mViewportHeight = 720;

		bool mPanning, mRotating;
		glm::vec2 mInitialMousePosition;
		glm::vec3 mInitialFocalPoint, mInitialRotation;

		float mDistance;

		float mPitch, mYaw;
	};
}