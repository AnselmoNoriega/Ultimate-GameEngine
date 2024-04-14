#include "nrpch.h"
#include "Application.h"

#include "NotRed/Core/Log.h"

#include "NotRed/Scripting/ScriptEngine.h"

#include "NotRed/Events/Event.h"
#include "NotRed/Events/ApplicationEvent.h"
#include "NotRed/Utils/PlatformUtils.h"

#include "Input.h"

#include "NotRed/Renderer/Renderer.h"

#include <GLFW/glfw3.h>
#include <filesystem>

namespace NR
{
#define BIND_EVENT_FN(x) [this](auto&&... args)-> decltype(auto) { return this->x(std::forward<decltype(args)>(args)...); }

    Application* Application::sInstance = nullptr;

    Application::Application(const ApplicationSpecification& specification)
        : mSpecification(specification)
    {
        NR_PROFILE_FUNCTION();

        NR_CORE_ASSERT(!sInstance, "There can only be one application");
        sInstance = this;

        if (!mSpecification.WorkingDirectory.empty())
        {
            std::filesystem::current_path(mSpecification.WorkingDirectory);
        }

        mWindow = Window::Create(WindowProps(mSpecification.Name));
        mWindow->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));

        Renderer::Init();
        ScriptEngine::Init();

        mImGuiLayer = new ImGuiLayer();
        PushOverlay(mImGuiLayer);
    }

    void Application::OnEvent(Event& e)
    {
        NR_PROFILE_FUNCTION();

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));
        dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Application::OnWindowResize));

        for (auto layer = mLayerStack.rbegin(); layer != mLayerStack.rend(); ++layer)
        {
            if (e.Handled)
            {
                break;
            }
            (*layer)->OnEvent(e);
        }
    }

    void Application::PushLayer(Layer* layer)
    {
        NR_PROFILE_FUNCTION();

        mLayerStack.PushLayer(layer);
        layer->Attach();
    }

    void Application::PushOverlay(Layer* overlay)
    {
        NR_PROFILE_FUNCTION();

        mLayerStack.PushOverlay(overlay);
        overlay->Attach();
    }

    void Application::Run()
    {
        NR_PROFILE_FUNCTION();

        while (mRunning)
        {
            NR_PROFILE_SCOPE("Run loop");

            float time = Time::GetTime();;
            mTimeStep.UpdateTimeFrame(time);

            if (!mMinimized)
            {
                NR_PROFILE_SCOPE("LayerStack Update");

                for (Layer* layer : mLayerStack)
                {
                    layer->Update(mTimeStep);
                }
            }

            mImGuiLayer->Begin();
            {
                NR_PROFILE_SCOPE("ImGUIStack Update");

                for (Layer* layer : mLayerStack)
                {
                    layer->ImGuiRender();
                }
            }
            mImGuiLayer->End();

            mWindow->Update();
        }
    }

    bool Application::OnWindowClose(WindowCloseEvent& e)
    {
        mRunning = false;
        return true;
    }

    bool Application::OnWindowResize(WindowResizeEvent& e)
    {
        NR_PROFILE_FUNCTION();

        if (e.GetWidth() == 0 || e.GetHeight() == 0)
        {
            mMinimized = true;
            return false;
        }

        mMinimized = false;
        Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());

        return false;
    }

    Application::~Application()
    {
        ScriptEngine::Shutdown();
        Renderer::Shutdown();
    }
}

