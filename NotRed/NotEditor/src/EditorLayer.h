#pragma once

#include "NotRed.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <string>

#include "imgui/imgui_internal.h"

#include "NotRed/ImGui/ImGuiLayer.h"
#include "NotRed/Editor/EditorCamera.h"

#include "NotRed/Editor/SceneHierarchyPanel.h"
#include "NotRed/Editor/ContentBrowserPanel.h"
#include "NotRed/Editor/ObjectsPanel.h"

namespace NR
{
	class EditorLayer : public Layer
	{
	public:
		enum class PropertyFlag
		{
			None, 
			ColorProperty, 
			DragProperty, 
			SliderProperty
		};

	public:
		EditorLayer();
		~EditorLayer();

		void Attach() override;
		void Detach() override;
		void Update(float dt) override;

		void ImGuiRender() override;
		bool OnKeyPressedEvent(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
		void OnEvent(Event& e) override;

		void ShowBoundingBoxes(bool show, bool onTop = false);
		void SelectEntity(Entity entity);

		void NewScene();
		void OpenProject();
		void OpenProject(const std::string& filepath);
		void OpenScene(const std::string& filepath);
		void OpenScene(const AssetMetadata& assetMetadata);
		void SaveScene();
		void SaveSceneAs();

	private:
		std::pair<float, float> GetMouseViewportSpace(bool primaryViewport);
		std::pair<glm::vec3, glm::vec3> CastRay(const EditorCamera& camera, float mx, float my);

		struct SelectedSubmesh
		{
			Entity EntityObj;
			Submesh* Mesh = nullptr;
			float Distance = 0.0f;
		};

		void Selected(const SelectedSubmesh& selectionContext);
		void EntityDeleted(Entity e);
		Ray CastMouseRay();

		void ScenePlay();
		void SceneStop();

		void UI_WelcomePopup();
		void UI_AboutPopup();

		void UpdateWindowTitle(const std::string& sceneName);

		float GetSnapValue();

		void DeleteEntity(Entity entity);

		void UpdateSceneRendererSettings();

	private:
		Scope<SceneHierarchyPanel> mSceneHierarchyPanel;
		Scope<ContentBrowserPanel> mContentBrowserPanel;
		Scope<ObjectsPanel> mObjectsPanel;

		Ref<Scene> mRuntimeScene, mEditorScene, mCurrentScene;
		Ref<SceneRenderer> mViewportRenderer;
		Ref<SceneRenderer> mSecondViewportRenderer;
		Ref<SceneRenderer> mFocusedRenderer;
		std::string mSceneFilePath;
		bool mReloadScriptOnPlay = true;

		EditorCamera mEditorCamera;
		EditorCamera mSecondEditorCamera;

		Ref<Shader> mBrushShader;
		Ref<Material> mSphereBaseMaterial;

		struct AlbedoInput
		{
			glm::vec3 Color = { 0.972f, 0.96f, 0.915f }; // Silver, from https://docs.unrealengine.com/en-us/Engine/Rendering/Materials/PhysicallyBased
			Ref<Texture2D> TextureMap;
			bool SRGB = true;
			bool UseTexture = false;
		};

		struct NormalInput
		{
			Ref<Texture2D> TextureMap;
			bool UseTexture = false;
		};

		struct MetalnessInput
		{
			float Value = 1.0f;
			Ref<Texture2D> TextureMap;
			bool UseTexture = false;
		};

		struct RoughnessInput
		{
			float Value = 0.2f;
			Ref<Texture2D> TextureMap;
			bool UseTexture = false;
		};

		float mEnvMapRotation = 0.0f;

		enum class SceneType : uint32_t
		{
			Spheres = 0, Model = 1
		};
		SceneType mSceneType;

		// Editor resources
		Ref<Texture2D> mCheckerboardTex;
		Ref<Texture2D> mPlayButtonTex, mStopButtonTex, mPauseButtonTex;

		glm::vec2 mViewportBounds[2];
		glm::vec2 mSecondViewportBounds[2];

		float mLineWidth = 2.0f;

		int mGizmoType = -1;

		float mSnapValue = 0.5f;
		float mRotationSnapValue = 45.0f;

		bool mDrawOnTopBoundingBoxes = false;

		bool mShowBoundingBoxes = false;
		bool mShowBoundingBoxSelectedMeshOnly = true;
		bool mShowBoundingBoxSubmeshes = false;
		bool mShowSelectedWireframe = true;
		bool mShowPhysicsCollidersWireframe = false;

		bool mViewportPanelMouseOver = false;
		bool mViewportPanelFocused = false;
		bool mAllowViewportCameraEvents = false;

		bool mViewportPanel2MouseOver = false;
		bool mViewportPanel2Focused = false;

		bool mShowPhysicsSettings = false;
		bool mShowSecondViewport = false;

		bool mShowWelcomePopup = true;
		bool mShowAboutPopup = false;

		enum class SceneState
		{
			Edit = 0, Play = 1, Pause = 2
		};
		SceneState mSceneState = SceneState::Edit;

		enum class SelectionMode
		{
			None = 0, Entity = 1, SubMesh = 2
		};

		SelectionMode mSelectionMode = SelectionMode::Entity;
		std::vector<SelectedSubmesh> mSelectionContext;
		glm::mat4* mRelativeTransform = nullptr;
		glm::mat4* mCurrentlySelectedTransform = nullptr;
	};

}