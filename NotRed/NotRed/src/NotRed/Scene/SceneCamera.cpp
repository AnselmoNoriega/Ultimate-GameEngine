#include "nrpch.h"
#include "SceneCamera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace NR
{
    void SceneCamera::SetPerspective(float verticalFOV, float nearClip, float farClip)
    {
        mProjectionType = ProjectionType::Perspective;
        mPerspectiveFOV = verticalFOV;
        mPerspectiveNear = nearClip;
        mPerspectiveFar = farClip;
    }

    void SceneCamera::SetOrthographic(float size, float nearClip, float farClip)
    {
        mProjectionType = ProjectionType::Orthographic;
        mOrthographicSize = size;
        mOrthographicNear = nearClip;
        mOrthographicFar = farClip;
    }

    void SceneCamera::SetViewportSize(uint32_t width, uint32_t height)
    {
        switch (mProjectionType)
        {
        case ProjectionType::Perspective:
        {
            mProjectionMatrix = glm::perspectiveFov(mPerspectiveFOV, (float)width, (float)height, mPerspectiveNear, mPerspectiveFar);
            break;
        }
        case ProjectionType::Orthographic:
        {
            float aspect = (float)width / (float)height;
            float width = mOrthographicSize * aspect;
            float height = mOrthographicSize;
            mProjectionMatrix = glm::ortho(-width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f);
            break;
        }
        }
    }
}