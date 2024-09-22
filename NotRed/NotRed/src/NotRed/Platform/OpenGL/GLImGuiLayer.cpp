#include "nrpch.h"
#include "GLImGuiLayer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#define IMGUI_IMPL_API
#include "backends/imgui_impl_glfw.h"
#include "NotRed/ImGui/ImGuizmo.h"

#include "backends/imgui_impl_opengl3.h"

#include "NotRed/Core/Application.h"

#include "NotRed/Renderer/Renderer.h"

namespace NR
{
	GLImGuiLayer::GLImGuiLayer()
	{

	}

	GLImGuiLayer::GLImGuiLayer(const std::string& name)
	{

	}

	GLImGuiLayer::~GLImGuiLayer()
	{

	}

	void GLImGuiLayer::Attach()
	{
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		//io.ConfigViewportsNoAutoMerge = true;
		//io.ConfigViewportsNoTaskBarIcon = true;

		io.Fonts->AddFontFromFileTTF("Assets/Fonts/Roboto/Roboto-Bold.ttf", 18.0f);
		io.Fonts->AddFontFromFileTTF("Assets/Fonts/Roboto/Roboto-Regular.ttf", 24.0f);
		io.FontDefault = io.Fonts->AddFontFromFileTTF("Assets/Fonts/Roboto/Roboto-Regular.ttf", 18.0f);

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		SetDarkThemeColors();
		//ImGui::StyleColorsClassic();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, style.Colors[ImGuiCol_WindowBg].w);

		Application& app = Application::Get();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

		// Setup Platform/Renderer bindings
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 450");
	}

	void GLImGuiLayer::Detach()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void GLImGuiLayer::Begin()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void GLImGuiLayer::End()
	{
		ImGuiIO& io = ImGui::GetIO();
		Application& app = Application::Get();
		io.DisplaySize = ImVec2(app.GetWindow().GetWidth(), app.GetWindow().GetHeight());

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Rendering
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	void GLImGuiLayer::ImGuiRender()
	{
	}
}