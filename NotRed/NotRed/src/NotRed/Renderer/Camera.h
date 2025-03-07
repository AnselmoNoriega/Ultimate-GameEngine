#pragma once

namespace NR
{
	class Camera
	{
	public:
		Camera() = default;
		Camera(const glm::mat4& projectionMatrix);

		virtual ~Camera() = default;

		const glm::mat4& GetProjectionMatrix() const { return mProjectionMatrix; }
		void SetProjectionMatrix(const glm::mat4& projectionMatrix) { mProjectionMatrix = projectionMatrix; }

		float GetExposure() const { return mExposure; }
		float& GetExposure() { return mExposure; }

	protected:
		glm::mat4 mProjectionMatrix = glm::mat4(1.0f);
		float mExposure = 0.8f;
	};
}