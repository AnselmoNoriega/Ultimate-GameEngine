#include "RuntimeLayer.h"

#include <filesystem>

#define GLmENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "NotRed/ImGui/ImGuizmo.h"

#include "imgui_internal.h"
#include "NotRed/ImGui/ImGui.h"

#include "NotRed/Renderer/Renderer2D.h"
#include "NotRed/Editor/AssetEditorPanel.h"
#include "NotRed/Script/ScriptEngine.h"

#include "NotRed/Physics/PhysicsManager.h"
#include "NotRed/Editor/PhysicsSettingsWindow.h"
#include "NotRed/Math/Math.h"

#include "NotRed/Renderer/RendererAPI.h"
#include "NotRed/Platform/OpenGL/GLFrameBuffer.h"
#include "NotRed/Util/FileSystem.h"

namespace NR
{
	RuntimeLayer::RuntimeLayer()
		: mEditorCamera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 1000.0f))
	{
	}

	void RuntimeLayer::Attach()
	{
		OpenScene("Assets/Scenes/Testing.nrsc");
		
		SceneRendererSpecification spec;
		spec.SwapChainTarget = true;
		mSceneRenderer = Ref<SceneRenderer>::Create(mRuntimeScene, spec);
		mSceneRenderer->GetOptions().ShowGrid = false;
		mSceneRenderer->SetShadowSettings(-50.0f, 50.0f, 0.95f);
		
		ScenePlay();
	}

	void RuntimeLayer::Detach()
	{
	}

	void RuntimeLayer::ScenePlay()
	{
		mRuntimeScene->RuntimeStart();
	}

	void RuntimeLayer::SceneStop()
	{

	}

	void RuntimeLayer::UpdateWindowTitle(const std::string& sceneName)
	{
		Application::Get().GetWindow().SetTitle(sceneName);
	}

	void RuntimeLayer::Update(float dt)
	{
		auto [width, height] = Application::Get().GetWindow().GetSize();
		mSceneRenderer->SetViewportSize(width, height);
		mRuntimeScene->SetViewportSize(width, height);

		if (mViewportPanelFocused)
		{
			mEditorCamera.Update(dt);
		}

		mRuntimeScene->Update(dt);
		mRuntimeScene->RenderRuntime(mSceneRenderer, dt);
	}

	void RuntimeLayer::ShowBoundingBoxes(bool show, bool onTop)
	{
		mSceneRenderer->GetOptions().ShowBoundingBoxes = show && !onTop;
		mDrawOnTopBoundingBoxes = show && onTop;
	}

	void RuntimeLayer::OpenScene(const std::string& filepath)
	{
		Ref<Scene> newScene = Ref<Scene>::Create("New Scene", false);

		SceneSerializer serializer(newScene);
		serializer.Deserialize(filepath);

		mRuntimeScene = newScene;
		mSceneFilePath = filepath;

		std::filesystem::path path = filepath;
		UpdateWindowTitle(path.filename().string());
		ScriptEngine::SetSceneContext(mRuntimeScene);
	}

	void RuntimeLayer::OnEvent(Event& e)
	{
		mRuntimeScene->OnEvent(e);
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>(NR_BIND_EVENT_FN(RuntimeLayer::OnKeyPressedEvent));
		dispatcher.Dispatch<MouseButtonPressedEvent>(NR_BIND_EVENT_FN(RuntimeLayer::OnMouseButtonPressed));
	}

	bool RuntimeLayer::OnKeyPressedEvent(KeyPressedEvent& e)
	{
		switch (e.GetKeyCode())
		{
		case KeyCode::Escape:
			break;
		}
		return false;
	}

	bool RuntimeLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		return false;
	}
}