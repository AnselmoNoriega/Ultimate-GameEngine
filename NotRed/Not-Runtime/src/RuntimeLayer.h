#pragma once

#include "NotRed.h"

#include <string>

#include "imgui/imgui_internal.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "NotRed/ImGui/ImGuiLayer.h"

#include "NotRed/Editor/EditorCamera.h"

#include "NotRed/Editor/SceneHierarchyPanel.h"

namespace NR
{
	class RuntimeLayer : public Layer
	{
	public:
		RuntimeLayer(std::string_view projectPath);
		~RuntimeLayer() override = default;

		void Attach() override;
		void Detach() override;

		void Update(float dt) override;
		void OnEvent(Event& e) override;
		bool OnKeyPressedEvent(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		void OpenProject();
		void OpenScene(const std::string& filepath);

	private:
		void ScenePlay();
		void SceneStop();
		void UpdateWindowTitle(const std::string& sceneName);
		
	private:
		Ref<Scene> mRuntimeScene;
		Ref<SceneRenderer> mSceneRenderer;
		Ref<Renderer2D> mRenderer2D;

		std::string mProjectPath;

		bool mReloadScriptOnPlay = true;

		glm::mat4 mRenderer2DProj;

		uint32_t mWidth = 0, mHeight = 0;

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