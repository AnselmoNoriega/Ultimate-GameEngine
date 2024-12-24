#include "nrpch.h"
#include "Application.h"

#include <filesystem>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <Windows.h>

#include <imgui/imgui.h>
#include "imgui/imgui_internal.h"

#include "NotRed/Renderer/Renderer.h"
#include "NotRed/Renderer/Framebuffer.h"
#include "NotRed/Script/ScriptEngine.h"
#include "NotRed/Physics/PhysicsManager.h"
#include "NotRed/Asset/AssetManager.h"

#include "NotRed/Audio/AudioEngine.h"

#include "NotRed/Platform/Vulkan/VkRenderer.h"
#include "NotRed/Platform/Vulkan/VKAllocator.h"
#include "NotRed/Platform/Vulkan/VKSwapChain.h"

#include "NotRed/Util/StringUtils.h"

#include "NotRed/Debug/Profiler.h"

extern bool gApplicationRunning;
extern ImGuiContext* GImGui;

namespace NR
{
#define BIND_EVENT_FN(fn) std::bind(&Application::##fn, this, std::placeholders::_1)

	Application* Application::sInstance = nullptr;

	Application::Application(const ApplicationSpecification& specification)
		: mSpecification(specification)
	{
		sInstance = this;

		if (!specification.WorkingDirectory.empty())
		{
			std::filesystem::current_path(specification.WorkingDirectory);
		}

		mProfiler = new PerformanceProfiler();

		WindowSpecification windowSpec;
		windowSpec.Title = specification.Name;
		windowSpec.Width = specification.WindowWidth;
		windowSpec.Height = specification.WindowHeight;
		windowSpec.Fullscreen = specification.Fullscreen;
		windowSpec.VSync = specification.VSync;
		mWindow = std::unique_ptr<Window>(Window::Create(windowSpec));

		mWindow->Init();
		mWindow->SetEventCallback([this](Event& e) { return OnEvent(e); });
		mWindow->Maximize();
		mWindow->SetVSync(true);

		Renderer::Init();
		Renderer::WaitAndRender();

		if (mSpecification.EnableImGui)
		{
			mImGuiLayer = ImGuiLayer::Create();
			PushOverlay(mImGuiLayer);
		}

		ScriptEngine::Init("Resources/Scripts/Not-ScriptCore.dll");
		PhysicsManager::Init();
		Audio::AudioEngine::Init();
	}

	Application::~Application()
	{
		for (Layer* layer : mLayerStack)
		{
			layer->Detach();
			delete layer;
		}

		FrameBufferPool::GetGlobal()->GetAll().clear();

		PhysicsManager::Shutdown();
		ScriptEngine::Shutdown();
		AssetManager::Shutdown();
		Audio::AudioEngine::Shutdown();

		Renderer::WaitAndRender();
		for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; ++i)
		{
			auto& queue = Renderer::GetRenderResourceReleaseQueue(i);
			queue.Execute();
		}
		Renderer::Shutdown();

		Project::SetActive(nullptr);

		delete mProfiler;
		mProfiler = nullptr;
	}

	void Application::RenderImGui()
	{
		NR_SCOPE_PERF("Application::RenderImGui");

		mImGuiLayer->Begin();

		{
			ImGui::Begin("Renderer");
			auto& caps = Renderer::GetCapabilities();
			ImGui::Text("Vendor: %s", caps.Vendor.c_str());
			ImGui::Text("Renderer: %s", caps.Device.c_str());
			ImGui::Text("Version: %s", caps.Version.c_str());
			ImGui::Separator();
			ImGui::Text("Frame Time: %.2fms\n", mTimeFrame.GetMilliseconds());

			if (RendererAPI::Current() == RendererAPIType::Vulkan)
			{
				GPUMemoryStats memoryStats = VKAllocator::GetStats();
				std::string used = Utils::BytesToString(memoryStats.Used);
				std::string free = Utils::BytesToString(memoryStats.Free);
				ImGui::Text("Used VRAM: %s", used.c_str());
				ImGui::Text("Free VRAM: %s", free.c_str());
			}
		}

		bool vsync = mWindow->IsVSync();
		if (ImGui::Checkbox("Vsync", &vsync))
		{
			mWindow->SetVSync(vsync);
		}

		ImGui::End();

		{
			ImGui::SetNextWindowSize(ImVec2(0.0f, 0.0f));
			ImGui::Begin("Audio Stats");

			ImGui::Separator();

			Audio::Stats audioStats = Audio::AudioEngine::GetStats();
			std::string active = std::to_string(audioStats.NumActiveSounds);

			std::string max = std::to_string(audioStats.TotalSources);
			std::string numAC = std::to_string(audioStats.NumAudioComps);
			std::string ramEn = Utils::BytesToString(audioStats.MemEngine);
			std::string ramRM = Utils::BytesToString(audioStats.MemResManager);

			ImGui::Text("Active Sounds: %s", active.c_str());
			ImGui::Text("Max Sources: %s", max.c_str());
			ImGui::Text("Audio Components: %s", numAC.c_str());

			ImGui::Separator();

			ImGui::Text("Frame Time: %.3fms\n", audioStats.FrameTime);
			ImGui::Text("Used RAM (Engine - backend): %s", ramEn.c_str());
			ImGui::Text("Used RAM (Resource Manager): %s", ramRM.c_str());

			ImGui::End();
		}
		{
			ImGui::Begin("Performance");
			ImGui::Text("Frame Time: %.2fms\n", mTimeFrame.GetMilliseconds());

			const auto& perFrameData = mProfiler->GetPerFrameData();
			for (auto&& [name, time] : perFrameData)
			{
				ImGui::Text("%s: %.3fms\n", name, time);
			}

			ImGui::End();
			mProfiler->Clear();
		}

		for (Layer* layer : mLayerStack)
		{
			layer->ImGuiRender();
		}
	}

	void Application::PushLayer(Layer* layer)
	{
		mLayerStack.PushLayer(layer);
		layer->Attach();
	}

	void Application::PushOverlay(Layer* layer)
	{
		mLayerStack.PushOverlay(layer);
		layer->Attach();
	}

	void Application::Run()
	{
		Init();

		while (mRunning)
		{
			NR_PROFILE_FRAME("MainThread");

			static uint64_t frameCounter = 0;
			mWindow->ProcessEvents();

			if (!mMinimized)
			{
				Renderer::BeginFrame();
				{
					NR_SCOPE_PERF("Application Layer::Update");
					for (Layer* layer : mLayerStack)
					{
						layer->Update((float)mTimeFrame);
					}
				}

				Application* app = this;

				if (mSpecification.EnableImGui)
				{
					Renderer::Submit([app]() { app->RenderImGui(); });
					Renderer::Submit([=]() {mImGuiLayer->End(); });
				}
				Renderer::EndFrame();

				mWindow->GetSwapChain().BeginFrame();
				Renderer::WaitAndRender();
				mWindow->SwapBuffers();
			}

			float time = GetTime();
			mTimeFrame = time - mLastFrameTime;
			mLastFrameTime = time;

			++frameCounter;
		}

		Shutdown();
	}

	void Application::Close()
	{
		mRunning = false;
	}

	void Application::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) { return OnWindowResize(e); });
		dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) { return OnWindowClose(e); });

		for (auto it = mLayerStack.end(); it != mLayerStack.begin(); )
		{
			(*--it)->OnEvent(event);
			if (event.Handled)
			{
				break;
			}
		}
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		const uint32_t width = e.GetWidth(), height = e.GetHeight();
		if (width == 0 || height == 0)
		{
			mMinimized = true;
			return false;
		}
		mMinimized = false;

		mWindow->GetSwapChain().Resize(width, height);

		return false;
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		mRunning = false;
		gApplicationRunning = false;
		return true;
	}

	std::string Application::OpenFile(const char* filter) const
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };

		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)mWindow->GetNativeWindow());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&ofn) == TRUE)
		{
			return ofn.lpstrFile;
		}
		return std::string();
	}

	std::string Application::SaveFile(const char* filter) const
	{
		OPENFILENAMEA ofn;       // common dialog box structure
		CHAR szFile[260] = { 0 };       // if using TCHAR macros

		// Initialize OPENFILENAME
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)mWindow->GetNativeWindow());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetSaveFileNameA(&ofn) == TRUE)
		{
			const char* ext = strrchr(filter, '.');
			if (ext != nullptr)
			{
				std::string filePath = ofn.lpstrFile;
				std::string extension = ext;

				if (filePath.find(extension) == std::string::npos)
				{
					filePath += extension;
				}

				return filePath;
			}

			return ofn.lpstrFile;
		}
		if (GetSaveFileNameA(&ofn) == TRUE)
		{
			return ofn.lpstrFile;
		}
		return std::string();
	}

	float Application::GetTime() const
	{
		return (float)glfwGetTime();
	}

	const char* Application::GetConfigurationName()
	{
#if defined(NR_DEBUG)
		return "Debug";
#elif defined(NR_RELEASE)
		return "Release";
#elif defined(NR_DIST)
		return "Dist";
#else
#error Undefined configuration?
#endif
	}

	const char* Application::GetPlatformName()
	{
#if defined(NR_PLATFORM_WINDOWS)
		return "Windows x64";
#else
#error Undefined platform?
#endif
	}
}