#include "nrpch.h"
#include "SceneCamera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace NR
{
    void SceneCamera::SetOrthographic(float size, float nearClip, float farClip)
    {
        mProjectionType = ProjectionType::Orthographic;

        mFOV = size;
        mNearClip = nearClip;
        mFarClip = farClip;

        RecalculateProjection();
    }

    void SceneCamera::SetPerspective(float fov, float nearClip, float farClip)
    {
        mProjectionType = ProjectionType::Perspective;

        mFOV = fov;
        mNearClip = nearClip;
        mFarClip = farClip;

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

    void SceneCamera::SetProjectionType(ProjectionType type)
    {
        mProjectionType = type;

        if (mProjectionType == ProjectionType::Orthographic)
        {
            mFOV = 10.0f;
            mNearClip = -1.0f;
            mFarClip = 1.0f;
        }
        else
        {
            mFOV = glm::radians(45.0f);
            mNearClip = 0.1f;
            mFarClip = 1000.0f;
        }

        RecalculateProjection();
    }

    void SceneCamera::RecalculateProjection()
    {
        if (mProjectionType == ProjectionType::Orthographic)
        {
            float orthoLeft = -mFOV * mAspectRatio * 0.5f;
            float orthoRight = mFOV * mAspectRatio * 0.5f;
            float orthoBottom = -mFOV * 0.5f;
            float orthoTop = mFOV * 0.5f;

            mProjection = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, mNearClip, mFarClip);
        }
        else
        {
            mProjection = glm::perspective(mFOV, mAspectRatio, mNearClip, mFarClip);
        }
    }
}