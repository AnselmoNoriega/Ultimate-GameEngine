#pragma once

#include "NotRed.h"

#include "NotRed/ImGui/ImGuiLayer.h"
#include "imgui/imgui_internal.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLmENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <string>

#include "NotRed/Editor/SceneHierarchyPanel.h"

namespace NR
{
	class EditorLayer : public Layer
	{
	public:
		enum class PropertyFlag
		{
			None, 
			ColorProperty
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
		void Property(const std::string& name, float& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
		void Property(const std::string& name, glm::vec2& value, PropertyFlag flags);
		void Property(const std::string& name, glm::vec2& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
		void Property(const std::string& name, glm::vec3& value, PropertyFlag flags);
		void Property(const std::string& name, glm::vec3& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);
		void Property(const std::string& name, glm::vec4& value, PropertyFlag flags);
		void Property(const std::string& name, glm::vec4& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None);

		void ShowBoundingBoxes(bool show, bool onTop = false);

	private:
		std::pair<float, float> GetMouseViewportSpace();
		std::pair<glm::vec3, glm::vec3> CastRay(float mx, float my);

		struct SelectedSubmesh
		{
			Entity EntityObj;
			Submesh* Mesh;
			float Distance;
		};
		void Selected(const SelectedSubmesh& selectionContext);
		Ray CastMouseRay();

	private:
		Scope<SceneHierarchyPanel> mSceneHierarchyPanel;

		Ref<Scene> mScene;
		Ref<Scene> mSphereScene;
		Ref<Scene> mActiveScene;

		Entity mMeshEntity;
		Entity mCameraEntity;

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
		AlbedoInput mAlbedoInput;

		struct NormalInput
		{
			Ref<Texture2D> TextureMap;
			bool UseTexture = false;
		};
		NormalInput mNormalInput;

		struct MetalnessInput
		{
			float Value = 1.0f;
			Ref<Texture2D> TextureMap;
			bool UseTexture = false;
		};
		MetalnessInput mMetalnessInput;

		struct RoughnessInput
		{
			float Value = 0.2f;
			Ref<Texture2D> TextureMap;
			bool UseTexture = false;
		};
		RoughnessInput mRoughnessInput;

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

		glm::vec2 mViewportBounds[2];
		float mSnapValue = 0.5f;

		int mGizmoType = -1;
		bool mAllowViewportCameraEvents = false;
		bool mDrawOnTopBoundingBoxes = false;

		bool mUIShowBoundingBoxes = false;
		bool mUIShowBoundingBoxesOnTop = false;

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