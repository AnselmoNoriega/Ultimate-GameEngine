#pragma once

#include "NotRed/Renderer/OrthographicCamera.h"

#include "NotRed/Events/ApplicationEvent.h"
#include "NotRed/Events/MouseEvent.h"

namespace NR
{
    class OrthographicCameraController
    {
    public:
        OrthographicCameraController(float aspectRatio);

        void Update(float deltaTime);
        void OnEvent(Event& e);

        OrthographicCamera& GetCamera() { return mCamera; }
        const OrthographicCamera& GetCamera() const { return mCamera; }

    private:
        bool MouseScrolled(MouseScrolledEvent& e);
        bool WindowResized(WindowResizeEvent& e);

    private:
        float mAspectRatio;
        float mZoomLevel = 1.0f;
        OrthographicCamera mCamera;

        glm::vec3 mCameraPosition = { 0.0f, 0.0f, 0.0f };
        float mCameraRotation = 0.0f;

        float mCameraMoveSpeed = 1.0f;
        float mCameraRotationSpeed = 15.0f;
    };
}

