#pragma once

#include "NotRed/Renderer/Camera.h"

namespace NR
{
	class SceneCamera : public Camera
	{
	public:
		enum class ProjectionType 
		{ 
			Perspective, 
			Orthographic
		};

	public:
		SceneCamera();
		virtual ~SceneCamera();

		void SetPerspective(float verticalFOV, float nearClip = 0.01f, float farClip = 10000.0f);
		void SetOrthographic(float size, float nearClip = -1.0f, float farClip = 1.0f);
		void SetViewportSize(uint32_t width, uint32_t height);

		void SetPerspectiveVerticalFOV(float verticalFov) { mPerspectiveFOV = glm::radians(verticalFov); }
		float GetPerspectiveVerticalFOV() const { return glm::degrees(mPerspectiveFOV); }
		void SetPerspectiveNearClip(float nearClip) { mPerspectiveNear = nearClip; }
		float GetPerspectiveNearClip() const { return mPerspectiveNear; }
		void SetPerspectiveFarClip(float farClip) { mPerspectiveFar = farClip; }
		float GetPerspectiveFarClip() const { return mPerspectiveFar; }

		void SetOrthographicSize(float size) { mOrthographicSize = size; }
		float GetOrthographicSize() const { return mOrthographicSize; }
		void SetOrthographicNearClip(float nearClip) { mOrthographicNear = nearClip; }
		float GetOrthographicNearClip() const { return mOrthographicNear; }
		void SetOrthographicFarClip(float farClip) { mOrthographicFar = farClip; }
		float GetOrthographicFarClip() const { return mOrthographicFar; }

		void SetProjectionType(ProjectionType type) { mProjectionType = type; }
		ProjectionType GetProjectionType() const { return mProjectionType; }

	private:
		ProjectionType mProjectionType = ProjectionType::Perspective;

		float mPerspectiveFOV = glm::radians(45.0f);
		float mPerspectiveNear = 0.1f, mPerspectiveFar = 1000.0f;

		float mOrthographicSize = 10.0f;
		float mOrthographicNear = -1.0f, mOrthographicFar = 1.0f;
	};

}