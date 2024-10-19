#include "RuntimeLayer.h"

#include <filesystem>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "NotRed/Project/Project.h"
#include "NotRed/Project/ProjectSerializer.h"

#include "NotRed/Script/ScriptEngine.h"

namespace NR
{
	RuntimeLayer::RuntimeLayer()
		: mEditorCamera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 1000.0f))
	{
	}

	void RuntimeLayer::Attach()
	{
		OpenProject("SandboxProject/Sandbox.nrproj");
		
		SceneRendererSpecification spec;
		spec.SwapChainTarget = true;
		mSceneRenderer = Ref<SceneRenderer>::Create(mRuntimeScene, spec);
		mSceneRenderer->GetOptions().ShowGrid = false;
		mSceneRenderer->SetShadowSettings(-50.0f, 50.0f, 0.95f);
		
		ScenePlay();
	}

	void RuntimeLayer::Detach()
	{
		SceneStop();
	}

	void RuntimeLayer::ScenePlay()
	{
		mRuntimeScene->RuntimeStart();
	}

	void RuntimeLayer::SceneStop()
	{
		mRuntimeScene->RuntimeStop();
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

	void RuntimeLayer::OpenProject(const std::string& filepath)
	{
		Ref<Project> project = Ref<Project>::Create();

		ProjectSerializer serializer(project);
		serializer.Deserialize(filepath);

		Project::SetActive(project);
		ScriptEngine::LoadAppAssembly((Project::GetScriptModuleFilePath()).string());
		
		mEditorCamera = EditorCamera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 1000.0f));
		if (!project->GetConfig().StartScene.empty())
		{
			OpenScene((Project::GetProjectDirectory() / project->GetConfig().StartScene).string());
		}
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