#pragma once

#include "NotRed/Renderer/Camera/Camera.h"

namespace NR
{
    class SceneCamera : public Camera
    {
    public:
        enum class ProjectionType { Perspective, Orthographic };

    public:
        SceneCamera() = default;
        ~SceneCamera() override = default;

        void SetOrthographic(float size, float nearClip, float farClip);
        void SetPerspective(float fov, float nearClip, float farClip);
        void SetViewportSize(uint32_t width, uint32_t height);

        float GetFOV() { return mFOV; }
        void SetFOV(float fov) { mFOV = fov; RecalculateProjection(); }

        float GetNearClip() { return mNearClip; }
        void SetNearClip(float nearClip) { mNearClip = nearClip; RecalculateProjection(); }

        float GetFarClip() { return mFarClip; }
        void SetFarClip(float farClip) { mFarClip = farClip; RecalculateProjection(); }

        ProjectionType GetProjectionType() { return mProjectionType; }
        void SetProjectionType(ProjectionType type);

    private:
        void RecalculateProjection();

    private:
        ProjectionType mProjectionType = ProjectionType::Orthographic;

        float mFOV = 10.0f,
              mNearClip = -1.0f,
              mFarClip = 1.0f;

        float mAspectRatio = 0.0f;
    };
}