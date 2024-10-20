#pragma once

#include "NotRed.h"

#include <string>

#include "imgui/imgui_internal.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLmENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "NotRed/ImGui/ImGuiLayer.h"

#include "NotRed/Editor/EditorCamera.h"

#include "NotRed/Editor/SceneHierarchyPanel.h"
#include "NotRed/Editor/ContentBrowserPanel.h"
#include "NotRed/Editor/ObjectsPanel.h"

namespace NR
{
	class RuntimeLayer : public Layer
	{
	public:
		RuntimeLayer();
		~RuntimeLayer() override = default;

		void Attach() override;
		void Detach() override;

		void Update(float dt) override;
		void OnEvent(Event& e) override;
		bool OnKeyPressedEvent(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		void ShowBoundingBoxes(bool show, bool onTop = false);

		void OpenProject(const std::string& filepath);
		void OpenScene(const std::string& filepath);

	private:
		void ScenePlay();
		void SceneStop();
		void UpdateWindowTitle(const std::string& sceneName);
		
	private:
		Ref<Scene> mRuntimeScene;
		Ref<SceneRenderer> mSceneRenderer;

		std::string mSceneFilePath;

		bool mReloadScriptOnPlay = true;

		EditorCamera mEditorCamera;
		
		bool mAllowViewportCameraEvents = false;
		bool mDrawOnTopBoundingBoxes = false;

		bool mUIShowBoundingBoxes = false;
		bool mUIShowBoundingBoxesOnTop = false;

		bool mViewportPanelMouseOver = false;
		bool mViewportPanelFocused = false;

		bool mShowPhysicsSettings = false;
	};
}