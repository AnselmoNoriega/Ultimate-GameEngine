#pragma once

#include "glm/glm.hpp"

namespace NR
{
	class Camera
	{
	public:
		Camera() = default;
		Camera(const glm::mat4& projectionMatrix);
		virtual ~Camera() = default;

		const glm::mat4& GetProjection() const { return mProjection; }
		void SetProjectionMatrix(const glm::mat4& projectionMatrix) { mProjection = projectionMatrix; }

		float GetExposure() const { return mExposure; }
		float& GetExposure() { return mExposure; }

	protected:
		glm::mat4 mProjection = glm::mat4(1.0f);
		float mExposure = 0.8f;
	};
}