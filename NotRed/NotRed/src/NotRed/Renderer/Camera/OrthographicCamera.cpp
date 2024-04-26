#include "nrpch.h"
#include "OrthographicCamera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace NR
{
    OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top)
        :mProjectionMatrix(glm::ortho(left, right, bottom, top, -1.0f, 1.0f)), mViewMatrix(1.0f)
    {
        NR_PROFILE_FUNCTION();

        mViewProjectionMatrix = mProjectionMatrix * mViewMatrix;
    }

    void OrthographicCamera::SetProjection(float left, float right, float bottom, float top)
    {
        NR_PROFILE_FUNCTION();

        mProjectionMatrix = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
        mViewProjectionMatrix = mProjectionMatrix * mViewMatrix;
    }

    void OrthographicCamera::RecalculateMatrix()
    {
        NR_PROFILE_FUNCTION();

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), mPosition);
        transform = glm::rotate(transform, glm::radians(mRotation), glm::vec3(0, 0, 1));

        mViewMatrix = glm::inverse(transform);
        mViewProjectionMatrix = mProjectionMatrix * mViewMatrix;
    }
}