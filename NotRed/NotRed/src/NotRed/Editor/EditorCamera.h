#pragma once

#include "NotRed/Renderer/Camera.h"
#include "NotRed/Core/TimeFrame.h"
#include "NotRed/Core/Events/KeyEvent.h"
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

        void Update(float dt);
        void OnEvent(Event& e);

        void Focus(const glm::vec3& focusPoint);
        void SetCameraMode(const CameraMode cameraMode) { mCameraMode = cameraMode; }

        bool IsActive() const { return mIsActive; }
        void SetActive(bool active) { mIsActive = active; }

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
        float& GetCameraSpeed() { return mSpeed; }
        [[nodiscard]] float GetCameraSpeed() const { return mSpeed; }
        void SetAllowed(const bool allowed) { mIsAllowed = allowed; }

    private:
        void CalculateYaw(float degrees);
        void CalculatePitch(float degrees);
        void UpdateCameraView();

        bool OnMouseScroll(MouseScrolledEvent& e);
        bool OnKeyPressed(KeyPressedEvent& e);
        bool OnKeyReleased(KeyReleasedEvent& e);

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

        bool mIsActive = true;
        bool mPanning, mRotating;
        glm::vec2 mInitialMousePosition {};
        glm::vec3 mInitialFocalPoint, mInitialRotation;

        float mDistance;
        float mSpeed = 0.03f;
        float mLastSpeed = 0.f;
        float mPitch, mYaw;

        bool mIsAllowed = false;

        glm::vec3 mCameraPositionDelta;
        glm::vec3 mRightDirection {};
        CameraMode mCameraMode = CameraMode::NONE;

        float mMinFocusDistance = 100.0f;

        uint32_t mViewportWidth = 1280, mViewportHeight = 720;
    };
}