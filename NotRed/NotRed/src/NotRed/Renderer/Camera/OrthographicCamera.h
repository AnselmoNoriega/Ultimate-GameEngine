#pragma once

#include <glm/glm.hpp>

namespace NR
{
    class OrthographicCamera
    {
    public:
        OrthographicCamera(
            float left = -1.0f, 
            float right = 1.0f, 
            float bottom = -1.0f, 
            float top = 1.0f
        );

        void SetProjection(
            float left,
            float right,
            float bottom,
            float top
        );

        const glm::vec3& GetPosition() const { return mPosition; }
        void SetPosition(const glm::vec3& position) 
        {
            mPosition = position;
            RecalculateMatrix();
        }

        float GetRotation() const { return mRotation; }
        void SetRotation(float rotation) 
        { 
            mRotation = rotation; 
            RecalculateMatrix();
        }

        const glm::mat4& GetVPMatrix() const { return mViewProjectionMatrix; }

    private:
        void RecalculateMatrix();

    private:
        glm::mat4 mViewProjectionMatrix;
        glm::mat4 mProjectionMatrix;
        glm::mat4 mViewMatrix;

        glm::vec3 mPosition = { 0.0f, 0.0f, 0.0f };
        float mRotation = 0.0f;
    };
}