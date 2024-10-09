#pragma once

#include "NotRed/Renderer/Camera.h"
#include "NotRed/Core/TimeFrame.h"
#include "NotRed/Core/Events/MouseEvent.h"

namespace NR
{
    class EditorCamera : public Camera
    {
    public:
        enum class CameraMode
        {
            NONE, FLYCAM, ARCBALL
        };

    public:
        EditorCamera() = default;
        EditorCamera(const glm::mat4& projectionMatrix);

        void CalculateYaw(float degrees);
        void CalculatePitch(float degrees);

        void Update(float dt);
        void OnEvent(Event& e);

        void Focus(const glm::vec3& focusPoint);
        void SetCameraMode(const CameraMode cameraMode);

        inline float GetDistance() const { return mDistance; }
        inline void SetDistance(float distance) { mDistance = distance; }

        inline void SetViewportSize(uint32_t width, uint32_t height) { mViewportWidth = width; mViewportHeight = height; }

        glm::vec3 GetUpDirection();
        glm::vec3 GetRightDirection();
        glm::vec3 GetForwardDirection();
        glm::quat GetOrientation() const;

        const glm::mat4& GetViewMatrix() const { return mViewMatrix; }
        glm::mat4 GetViewProjection() const { return mProjectionMatrix * mViewMatrix; }
        const glm::vec3& GetPosition() const { return mPosition; }

        float GetPitch() const { return mPitch; }
        float GetYaw() const { return mYaw; }

    private:
        void UpdateCameraView();

        bool OnMouseScroll(MouseScrolledEvent& e);

        void MousePan(const glm::vec2& delta);
        void MouseRotate(const glm::vec2& delta);
        void MouseZoom(float delta);

        glm::vec3 CalculatePosition();

        std::pair<float, float> PanSpeed() const;
        float RotationSpeed() const;
        float ZoomSpeed() const;

    private:
        glm::mat4 mViewMatrix;
        glm::vec3 mPosition, mRotation, mFocalPoint;

        bool mPanning, mRotating;
        glm::vec2 mInitialMousePosition {};
        glm::vec3 mInitialFocalPoint, mInitialRotation;

        float mDistance;
        float mSpeed = 0.03f;
        float mPitch, mYaw;

        glm::vec3 mCameraPositionDelta;
        glm::vec3 mRightDirection {};
        CameraMode mCameraMode = CameraMode::NONE;

        float mMinFocusDistance = 100.0f;

        uint32_t mViewportWidth = 1280, mViewportHeight = 720;
    };
}