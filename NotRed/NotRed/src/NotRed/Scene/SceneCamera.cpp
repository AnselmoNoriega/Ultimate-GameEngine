#include "nrpch.h"
#include "SceneCamera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace NR
{
    SceneCamera::SceneCamera()
    {

    }

    void SceneCamera::SetOrthographic(float size, float nearClip, float farClip)
    {
        mOrthographicSize = size;
        mOrthographicNear = nearClip;
        mOrthographicFar = farClip;

        RecalculateProjection();
    }

    void SceneCamera::SetViewportSize(uint32_t width, uint32_t height)
    {
        float aspectRatio = (float)width / (float)height;
        if (mAspectRatio != aspectRatio)
        {
            mAspectRatio = aspectRatio;
            RecalculateProjection();
        }
    }

    void SceneCamera::SetOrthographicSize(float size)
    {
        mOrthographicSize = size;

        RecalculateProjection();
    }

    void SceneCamera::RecalculateProjection()
    {
        float orthoLeft = -mOrthographicSize * mAspectRatio * 0.5f;
        float orthoRight = mOrthographicSize * mAspectRatio * 0.5f;
        float orthoBottom = -mOrthographicSize * 0.5f;
        float orthoTop = mOrthographicSize * 0.5f;

        mProjection = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, mOrthographicNear, mOrthographicFar);
    }
}