#pragma once

#include "NotRed/Core/UUID.h"
#include "NotRed/Core/TimeFrame.h"

#include "NotRed/Renderer/Camera.h"
#include "NotRed/Renderer/Texture.h"
#include "NotRed/Renderer/Material.h"
#include "NotRed/Renderer/SceneEnvironment.h"

#include "Entt/include/entt.hpp"

#include "SceneCamera.h"
#include "NotRed/Editor/EditorCamera.h"

namespace NR
{
	class SceneRenderer;
	class Prefab;

	struct DirLight
	{
		glm::vec3 Direction = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };

		float Multiplier = 1.0f;
	};

	struct DirectionalLight
	{
		glm::vec3 Direction = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };
		float Multiplier = 0.0f;

		bool CastShadows = true;
	};

	struct PointLight
	{
		glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
		float Multiplier = 0.0f;
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };
		float MinRadius = 0.001f;
		float Radius = 25.0f;
		float Falloff = 1.f;
		float SourceSize = 0.1f;
		bool CastsShadows = true;
		char Padding[3]{ 0, 0, 0 };
	};

	struct LightEnvironment
	{
		DirectionalLight DirectionalLights[4];
		std::vector<PointLight> PointLights;
	};


	class Entity;
	struct TransformComponent;

	using EntityMap = std::unordered_map<UUID, Entity>;

	class Scene : public Asset
	{
	public:
		Scene(const std::string& debugName = "Scene", bool isEditorScene = false, bool construct = true);
		~Scene();

		void Init();

		void Update(float dt);
		void RenderRuntime(Ref<SceneRenderer> renderer, float dt);
		void RenderEditor(Ref<SceneRenderer> renderer, float dt, const EditorCamera& editorCamera);
		void RenderSimulation(Ref<SceneRenderer> renderer, float ts, const EditorCamera& editorCamera);
		void OnEvent(Event& e);

		// Runtime
		void RuntimeStart();
		void RuntimeStop();
		void SimulationStart();
		void SimulationEnd();

		void SetViewportSize(uint32_t width, uint32_t height);

		const Ref<Environment>& GetEnvironment() const { return mEnvironment; }
		void SetSkybox(const Ref<TextureCube>& skybox);

		DirLight& GetLight() { return mLight; }
		const DirLight& GetLight() const { return mLight; }

		Entity GetMainCameraEntity();

		float& GetSkyboxLod() { return mSkyboxLod; }
		float GetSkyboxLod() const { return mSkyboxLod; }

		Entity CreateEntity(const std::string& name = "Entity");
		Entity CreateEntityWithID(UUID uuid, const std::string& name = "Entity", bool runtimeMap = false);
		void DestroyEntity(Entity entity);

		Entity DuplicateEntity(Entity entity);
		Entity CreatePrefabEntity(Entity entity);
		Entity Instantiate(Ref<Prefab> prefab);

		template<typename T>
		auto GetAllEntitiesWith()
		{
			return mRegistry.view<T>();
		}

		Entity FindEntityByTag(const std::string& tag);
		Entity FindEntityByID(UUID id);

		void ConvertToLocalSpace(Entity entity);
		void ConvertToWorldSpace(Entity entity);
		glm::mat4 GetTransformRelativeToParent(Entity entity);
		glm::vec3 GetTranslationRelativeToParent(Entity entity);
		glm::mat4 GetWorldSpaceTransformMatrix(Entity entity);
		TransformComponent GetWorldSpaceTransform(Entity entity);

		void ParentEntity(Entity entity, Entity parent);
		void UnparentEntity(Entity entity, bool convertToWorldSpace = true);

		const EntityMap& GetEntityMap() const { return mEntityIDMap; }
		void CopyTo(Ref<Scene>& target);

		UUID GetID() const { return mSceneID; }

		static Ref<Scene> GetScene(UUID uuid);

		bool IsEditorScene() const { return mIsEditorScene; }
		bool IsPlaying() const { return mIsPlaying; }

		float GetPhysics2DGravity() const;
		void SetPhysics2DGravity(float gravity);

		// Editor-specific
		void SetSelectedEntity(entt::entity entity) { mSelectedEntity = entity; }

		static AssetType GetStaticType() { return AssetType::Scene; }
		virtual AssetType GetAssetType() const override { return AssetType::Scene; }

		const std::string& GetName() const { return mName; }
		void SetName(const std::string& name) { mName = name; }
	
	public:
		static Ref<Scene> CreateEmpty();

	private:
		UUID mSceneID;
		entt::entity mSceneEntity;
		entt::registry mRegistry;

		std::string mName = "UntitledScene";
		std::string mDebugName;
		uint32_t mViewportWidth = 0, mViewportHeight = 0;

		EntityMap mEntityIDMap;

		DirLight mLight;
		float mLightMultiplier = 0.3f;

		LightEnvironment mLightEnvironment;

		Ref<Environment> mEnvironment;
		float mEnvironmentIntensity = 1.0f;
		Ref<TextureCube> mSkyboxTexture;
		Ref<Material> mSkyboxMaterial;

		entt::entity mSelectedEntity;

		Entity* mPhysics2DBodyEntityBuffer = nullptr;

		float mSkyboxLod = 1.0f;
		bool mIsPlaying = false;
		bool mShouldSimulate = false;
		bool mIsEditorScene = false;

	private:
		friend class Entity;
		friend class Prefab;
		friend class SceneRenderer;
		friend class SceneSerializer;
		friend class PrefabSerializer;
		friend class SceneHierarchyPanel;

		friend void ScriptComponentConstruct(entt::registry& registry, entt::entity entity);
		friend void ScriptComponentDestroy(entt::registry& registry, entt::entity entity);
		friend void AudioComponentConstruct(entt::registry& registry, entt::entity entity);
		friend void AudioComponentDestroy(entt::registry& registry, entt::entity entity);
	};

}