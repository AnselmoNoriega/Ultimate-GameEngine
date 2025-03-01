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

#include "NotRed/Editor/PanelManager.h"

#include "NotRed/Editor/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/ProjectSettingsWindow.h"
#include "NotRed/Editor/EditorConsolePanel.h"

#include "NotRed/Project/UserPreferences.h"

#include "NotRed/Renderer/UI/Font.h"
#include "NotRed/Editor/ECSPanel.h"

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
		EditorLayer(const Ref<UserPreferences>& userPreferences);
		~EditorLayer() override;

		void Attach() override;
		void Detach() override;
		void Update(float dt) override;

		void ImGuiRender() override;
		bool OnKeyPressedEvent(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
		void OnEvent(Event& e) override;

		void Render2D();

		void SelectEntity(Entity entity);

		void NewScene(const std::string& name = "UntitledScene");
		void OpenProject();
		void OpenProject(const std::string& filepath);
		void CreateProject(std::filesystem::path projectPath);
		void SaveProject();
		void CloseProject(bool unloadProject = true);
		void OpenScene(const std::string& filepath);
		void OpenScene(const AssetMetadata& assetMetadata);
		void OpenScene();
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

		void UI_CreateNewMeshPopup();
		void UI_InvalidAssetMetadataPopup();

		void UI_NewScene();

		void CreateMeshFromMeshSource(Entity entity, Ref<MeshSource> meshSource);
		void SceneHierarchyInvalidMetadataCallback(Entity entity, AssetHandle handle);

		void UpdateWindowTitle(const std::string& sceneName);
		void UI_DrawMenubar();

		// Returns titlebar height
		float UI_DrawTitlebar();
		void UI_HandleManualWindowResize();
		bool UI_TitleBarHitTest(int x, int y) const;

		float GetSnapValue();

		void DeleteEntity(Entity entity);

		void UpdateSceneRendererSettings();

	private:
		Ref<UserPreferences> mUserPreferences;		
		Scope<PanelManager> mPanelManager;

		Scope<ECSPanel> mECSDebugPanel;
		bool mECSDebugPanelOpen = false;

		Ref<Scene> mRuntimeScene, mEditorScene, mCurrentScene;
		Ref<SceneRenderer> mViewportRenderer;
		Ref<SceneRenderer> mSecondViewportRenderer;
		Ref<SceneRenderer> mFocusedRenderer;
		Ref<Renderer2D> mRenderer2D;
		std::string mSceneFilePath;

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

		// Editor resources
		Ref<Texture2D> mCheckerboardTex;
		Ref<Texture2D> mPlayButtonTex, mStopButtonTex, mPauseButtonTex;
		Ref<Texture2D> mSelectToolTex, mMoveToolTex, mRotateToolTex, mScaleToolTex;
		Ref<Texture2D> mIconMinimize, mIconMaximize, mIconRestore, mIconClose;
		Ref<Texture2D> mLogoTex;
		
		// Icons
		Ref<Texture2D> mPointLightIcon;

		glm::vec2 mViewportBounds[2];
		glm::vec2 mSecondViewportBounds[2];

		float mLineWidth = 2.0f;

		bool mTitleBarHovered = false;

		int mGizmoType = -1;

		float mSnapValue = 0.5f;
		float mRotationSnapValue = 45.0f;

		bool mDrawOnTopBoundingBoxes = true;

		bool mShowBoundingBoxes = false;
		bool mShowBoundingBoxSelectedMeshOnly = true;
		bool mShowBoundingBoxSubmeshes = false;
		bool mShowSelectedWireframe = false;
		bool mShowPhysicsCollidersWireframe = true;
		bool mShowPhysicsCollidersWireframeOnTop = false;

		bool mShowAudioEventsEditor = false;
		bool mAssetManagerPanelOpen = false;

		bool mShowIcons = true;

		bool mViewportPanelMouseOver = false;
		bool mViewportPanelFocused = false;
		bool mViewportHasCapturedMouse = false;

		bool mViewportPanel2MouseOver = false;
		bool mViewportPanel2Focused = false;

		bool mShowSecondViewport = false;

		bool mShowWelcomePopup = true;
		bool mShowAboutPopup = false;
		bool mShowCreateNewProjectPopup = false;

		bool mShowNewScenePopup = false;

		bool mShowCreateNewMeshPopup = false;

		struct CreateNewMeshPopupData
		{
			Ref<MeshSource> MeshToCreate;
			std::array<char, 256> CreateMeshFilenameBuffer;
			Entity TargetEntity;

			CreateNewMeshPopupData()
			{
				CreateMeshFilenameBuffer.fill(0);
				MeshToCreate = nullptr;
				TargetEntity = {};
			}

		} mCreateNewMeshPopupData;

		bool mShowInvalidAssetMetadataPopup = false;

		struct InvalidAssetMetadataPopupData
		{
			AssetMetadata Metadata;
		} mInvalidAssetMetadataPopupData;

		enum class SceneState
		{
			Edit, Play, Pause
		};
		SceneState mSceneState = SceneState::Edit;

		enum class SelectionMode
		{
			None, Entity, SubMesh
		};

		SelectionMode mSelectionMode = SelectionMode::Entity;
		std::vector<SelectedSubmesh> mSelectionContext;
		glm::mat4* mRelativeTransform = nullptr;
		glm::mat4* mCurrentlySelectedTransform = nullptr;
	};
}