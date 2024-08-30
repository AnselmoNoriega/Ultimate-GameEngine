#pragma once

#include "NotRed.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <string>

#include "NotRed/ImGui/ImGuiLayer.h"
#include "imgui/imgui_internal.h"

#include "NotRed/Editor/SceneHierarchyPanel.h"
#include "NotRed/Editor/EditorCamera.h"

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
		virtual ~EditorLayer();

		void Attach() override;
		void Detach() override;
		void Update(float dt) override;

		void ImGuiRender() override;
		void OnEvent(Event& e) override;

		bool OnKeyPressedEvent(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		// ImGui UI helpers
		bool Property(const std::string& name, bool& value);
		bool Property(const std::string& name, float& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
		bool Property(const std::string& name, glm::vec2& value, PropertyFlag flags);
		bool Property(const std::string& name, glm::vec2& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
		bool Property(const std::string& name, glm::vec3& value, PropertyFlag flags);
		bool Property(const std::string& name, glm::vec3& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
		bool Property(const std::string& name, glm::vec4& value, PropertyFlag flags);
		bool Property(const std::string& name, glm::vec4& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);

		void ShowBoundingBoxes(bool show, bool onTop = false);
		void SelectEntity(Entity entity);

		void OpenScene();
		void SaveScene();
		void SaveSceneAs();

	private:
		std::pair<float, float> GetMouseViewportSpace();
		std::pair<glm::vec3, glm::vec3> CastRay(float mx, float my);

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

		void UpdateWindowTitle(const std::string& sceneName);

		float GetSnapValue();

	private:
		Scope<SceneHierarchyPanel> mSceneHierarchyPanel;

		Ref<Scene> mRuntimeScene, mEditorScene;
		std::string mSceneFilePath;
		bool mReloadScriptOnPlay = true;

		EditorCamera mEditorCamera;

		Ref<Shader> mBrushShader;
		Ref<Material> mSphereBaseMaterial;

		Ref<Material> mMeshMaterial;
		std::vector<Ref<MaterialInstance>> mMetalSphereMaterialInstances;
		std::vector<Ref<MaterialInstance>> mDielectricSphereMaterialInstances;

		struct AlbedoInput
		{
			glm::vec3 Color = { 0.972f, 0.96f, 0.915f }; 
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

		bool mRadiancePrefilter = false;

		float mEnvMapRotation = 0.0f;

		enum class SceneType : uint32_t
		{
			Spheres, 
			Model
		};
		SceneType mSceneType;

		// Editor resources
		Ref<Texture2D> mCheckerboardTex;
		Ref<Texture2D> mPlayButtonTex;

		glm::vec2 mViewportBounds[2];
		float mSnapValue = 0.5f;
		float mRotationSnapValue = 45.0f;

		int mGizmoType = -1;
		bool mAllowViewportCameraEvents = false;
		bool mDrawOnTopBoundingBoxes = false;

		bool mUIShowBoundingBoxes = false;
		bool mUIShowBoundingBoxesOnTop = false;

		bool mViewportPanelMouseOver = false;
		bool mViewportPanelFocused = false;

		bool mShowPhysicsSettings = false;

		enum class SceneState
		{
			Edit,
			Play,
			Pause
		};
		SceneState mSceneState = SceneState::Edit;

		enum class SelectionMode
		{
			None, 
			Entity,
			SubMesh
		};

		SelectionMode mSelectionMode = SelectionMode::Entity;
		std::vector<SelectedSubmesh> mSelectionContext;
		glm::mat4* mRelativeTransform = nullptr;
		glm::mat4* mCurrentlySelectedTransform = nullptr;
	};

}