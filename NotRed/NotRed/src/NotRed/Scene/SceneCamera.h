#pragma once

#include "NotRed/Renderer/Camera.h"

namespace NR
{
    class SceneCamera : public Camera
    {
    public:
        SceneCamera();
        ~SceneCamera() override = default;

        void SetOrthographic(float size, float nearClip, float farClip);
        void SetViewportSize(uint32_t width, uint32_t height);

        void SetOrthographicSize(float size);

    private:
        void RecalculateProjection();

    private:
        float mOrthographicSize = 10.0f,
              mOrthographicNear = -1.0f,
              mOrthographicFar = 1.0f;

        float mAspectRatio = 0.0f;
    };
}