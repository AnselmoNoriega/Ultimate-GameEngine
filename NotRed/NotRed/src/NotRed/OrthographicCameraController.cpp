#include "nrpch.h"
#include "OrthographicCameraController.h"

#include "NotRed/Input.h"
#include "NotRed/KeyCodes.h"

namespace NR
{
    OrthographicCameraController::OrthographicCameraController(float aspectRatio)
        : mAspectRatio(aspectRatio), mCamera(-mAspectRatio * mZoomLevel, mAspectRatio * mZoomLevel, -mZoomLevel, mZoomLevel)
    {
    }

    void OrthographicCameraController::Update(float deltaTime)
    {
        if (Input::IsKeyPressed(NR_KEY_A))
        {
            mCameraPosition.x -= mCameraMoveSpeed * deltaTime;
            mCamera.SetPosition(mCameraPosition);
        }
        else if (Input::IsKeyPressed(NR_KEY_D))
        {
            mCameraPosition.x += mCameraMoveSpeed * deltaTime;
            mCamera.SetPosition(mCameraPosition);
        }

        if (Input::IsKeyPressed(NR_KEY_S))
        {
            mCameraPosition.y -= mCameraMoveSpeed * deltaTime;
            mCamera.SetPosition(mCameraPosition);
        }
        else if (Input::IsKeyPressed(NR_KEY_W))
        {
            mCameraPosition.y += mCameraMoveSpeed * deltaTime;
            mCamera.SetPosition(mCameraPosition);
        }

        if (Input::IsKeyPressed(NR_KEY_E))
        {
            mCameraRotation += mCameraRotationSpeed * deltaTime;
            mCamera.SetRotation(mCameraRotation);
        }
        else if (Input::IsKeyPressed(NR_KEY_Q))
        {
            mCameraRotation -= mCameraRotationSpeed * deltaTime;
            mCamera.SetRotation(mCameraRotation);
        }
    }

    void OrthographicCameraController::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseScrolledEvent>(NR_BIND_EVENT_FN(OrthographicCameraController::MouseScrolled));
        dispatcher.Dispatch<WindowResizeEvent>(NR_BIND_EVENT_FN(OrthographicCameraController::WindowResized));
    }

    bool OrthographicCameraController::MouseScrolled(MouseScrolledEvent& e)
    {
        mZoomLevel -= e.GetY_Offset() * 0.25f;
        mZoomLevel = std::max(mZoomLevel, 0.25f);
        mCameraMoveSpeed = mZoomLevel;
        mCamera.SetProjection(-mAspectRatio * mZoomLevel, mAspectRatio * mZoomLevel, -mZoomLevel, mZoomLevel);
        return false;
    }

    bool OrthographicCameraController::WindowResized(WindowResizeEvent& e)
    {
        mAspectRatio = static_cast<float>(e.GetWidth()) / static_cast<float>(e.GetHeight());
        mCamera.SetProjection(-mAspectRatio * mZoomLevel, mAspectRatio * mZoomLevel, -mZoomLevel, mZoomLevel);
        return false;
    }
}