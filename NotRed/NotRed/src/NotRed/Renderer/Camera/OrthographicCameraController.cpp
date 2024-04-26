#include "nrpch.h"
#include "OrthographicCameraController.h"

#include "NotRed/Core/Input.h"

namespace NR
{
    OrthographicCameraController::OrthographicCameraController(float aspectRatio)
        : mAspectRatio(aspectRatio), mCamera(-mAspectRatio * mZoomLevel, mAspectRatio * mZoomLevel, -mZoomLevel, mZoomLevel)
    {
    }

    void OrthographicCameraController::Update(float deltaTime)
    {
        NR_PROFILE_FUNCTION();

        if (Input::IsKeyPressed(KeyCode::A))
        {
            mCameraPosition.x -= mCameraMoveSpeed * deltaTime;
            mCamera.SetPosition(mCameraPosition);
        }
        else if (Input::IsKeyPressed(KeyCode::D))
        {
            mCameraPosition.x += mCameraMoveSpeed * deltaTime;
            mCamera.SetPosition(mCameraPosition);
        }

        if (Input::IsKeyPressed(KeyCode::S))
        {
            mCameraPosition.y -= mCameraMoveSpeed * deltaTime;
            mCamera.SetPosition(mCameraPosition);
        }
        else if (Input::IsKeyPressed(KeyCode::W))
        {
            mCameraPosition.y += mCameraMoveSpeed * deltaTime;
            mCamera.SetPosition(mCameraPosition);
        }

        if (Input::IsKeyPressed(KeyCode::E))
        {
            mCameraRotation += mCameraRotationSpeed * deltaTime;
            mCamera.SetRotation(mCameraRotation);
        }
        else if (Input::IsKeyPressed(KeyCode::Q))
        {
            mCameraRotation -= mCameraRotationSpeed * deltaTime;
            mCamera.SetRotation(mCameraRotation);
        }
    }

    void OrthographicCameraController::OnEvent(Event& e)
    {
        NR_PROFILE_FUNCTION();

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseScrolledEvent>(NR_BIND_EVENT_FN(OrthographicCameraController::MouseScrolled));
        dispatcher.Dispatch<WindowResizeEvent>(NR_BIND_EVENT_FN(OrthographicCameraController::WindowResized));
    }

    void OrthographicCameraController::Resize(float width, float height)
    {
        mAspectRatio = width / height;
        mCamera.SetProjection(-mAspectRatio * mZoomLevel, mAspectRatio * mZoomLevel, -mZoomLevel, mZoomLevel);
    }

    bool OrthographicCameraController::MouseScrolled(MouseScrolledEvent& e)
    {
        NR_PROFILE_FUNCTION();

        mZoomLevel -= e.GetY_Offset() * 0.25f;
        mZoomLevel = std::max(mZoomLevel, 0.25f);
        mCameraMoveSpeed = mZoomLevel;
        mCamera.SetProjection(-mAspectRatio * mZoomLevel, mAspectRatio * mZoomLevel, -mZoomLevel, mZoomLevel);
        return false;
    }

    bool OrthographicCameraController::WindowResized(WindowResizeEvent& e)
    {
        NR_PROFILE_FUNCTION();

        Resize((float)e.GetWidth(), (float)e.GetHeight());
        return false;
    }
}