#pragma once

#include "NotRed/Core/UUID.h"

#include "NotRed/Renderer/Camera.h"
#include "NotRed/Renderer/Texture.h"
#include "NotRed/Renderer/Material.h"
#include "NotRed/Renderer/SceneEnvironment.h"
#include "NotRed/Renderer/RenderCommandBuffer.h"
#include "NotRed/Renderer/Renderer2D.h"

#include "entt/include/entt.hpp"

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

		// C++ only
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
	using EntityMap = std::unordered_map<UUID, Entity>;

	struct TransformComponent;

	class PhysicsScene;

	class Scene : public Asset
	{
	public:
		Scene(const std::string& name = "UntitledScene", bool isEditorScene = false, bool initalize = true);
		~Scene();

		void Init();

		void UpdateRuntime(float dt);
		void UpdateEditor(float dt);

		void RenderRuntime(Ref<SceneRenderer> renderer, float dt);
		void RenderEditor(Ref<SceneRenderer> renderer, float dt, const EditorCamera& editorCamera);
		void RenderSimulation(Ref<SceneRenderer> renderer, float dt, const EditorCamera& editorCamera);

		void RenderPhysicsDebug(Ref<SceneRenderer> renderer, bool runtime);

		void OnEvent(Event& e);

		// Runtime
		void RuntimeStart();
		void RuntimeStop();

		void SimulationStart();
		void SimulationStop();

		void SetViewportSize(uint32_t width, uint32_t height);

		const Ref<Environment>& GetEnvironment() const { return mEnvironment; }
		void SetSkybox(const Ref<TextureCube>& skybox);

		DirLight& GetLight() { return mLight; }
		const DirLight& GetLight() const { return mLight; }

		Entity GetMainCameraEntity();

		float& GetSkyboxLod() { return mSkyboxLod; }
		float GetSkyboxLod() const { return mSkyboxLod; }

		Entity CreateEntity(const std::string& name = "");
		Entity CreateChildEntity(Entity parent, const std::string& name = "");
		Entity CreateEntityWithID(UUID uuid, const std::string& name = "", bool runtimeMap = false);
		void SubmitToDestroyEntity(Entity entity);
		void DestroyEntity(Entity entity, bool excludeChildren = false, bool first = true);

		void ResetTransformsToMesh(Entity entity, bool resetChildren);

		Entity DuplicateEntity(Entity entity);
		Entity CreatePrefabEntity(Entity entity, const glm::vec3* translation = nullptr, const glm::vec3* rotation = nullptr, const glm::vec3* scale = nullptr);
		Entity CreatePrefabEntity(Entity entity, Entity parent, const glm::vec3* translation = nullptr, const glm::vec3* rotation = nullptr, const glm::vec3* scale = nullptr);

		Entity Instantiate(Ref<Prefab> prefab, const glm::vec3* translation = nullptr, const glm::vec3* rotation = nullptr, const glm::vec3* scale = nullptr);
		Entity InstantiateMesh(Ref<Mesh> mesh);

		std::vector<UUID> FindBoneEntityIds(Entity parent, Ref<Mesh> mesh);

		template<typename T>
		auto GetAllEntitiesWith()
		{
			return mRegistry.view<T>();
		}

		Entity FindEntityByTag(const std::string& tag);
		Entity FindChildEntityByTag(Entity entity, const std::string& tag);
		Entity FindEntityByID(UUID id);

		void ConvertToLocalSpace(Entity entity);
		void ConvertToWorldSpace(Entity entity);
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

		//Box2DWorldComponent* GetWorld2D() const { return };
		float GetPhysics2DGravity() const;
		void SetPhysics2DGravity(float gravity);

		Ref<PhysicsScene> GetPhysicsScene() const;

		void SceneTransition(const std::string& scene);

		// Editor-specific
		void SetSelectedEntity(entt::entity entity) { mSelectedEntity = entity; }

		static AssetType GetStaticType() { return AssetType::Scene; }
		AssetType GetAssetType() const override { return GetStaticType(); }

		const std::string& GetName() const { return mName; }
		void SetName(const std::string& name) { mName = name; }

		void SetSceneTransitionCallback(const std::function<void(const std::string&)>& callback) { mSceneTransitionCallback = callback; }

	public:
		static Ref<Scene> CreateEmpty();

	private:
		void ScriptComponentConstruct(entt::registry& registry, entt::entity entity);
		void ScriptComponentDestroy(entt::registry& registry, entt::entity entity);
		void AudioComponentConstruct(entt::registry& registry, entt::entity entity);
		void AudioComponentDestroy(entt::registry& registry, entt::entity entity);
		void MeshColliderComponentConstruct(entt::registry& registry, entt::entity entity);
		void MeshColliderComponentDestroy(entt::registry& registry, entt::entity entity);

		void BuildMeshEntityHierarchy(Entity parent, Ref<Mesh> mesh, const void* assimpScene, void* assimpNode);
		void BuildMeshBoneEntityIds(Entity root, Entity entity);

		template<typename Fn>
		void SubmitPostUpdateFunc(Fn&& func)
		{
			mPostUpdateQueue.emplace_back(func);
		}

		ozz::vector<ozz::math::Float4x4> GetModelSpaceBoneTransforms(const std::vector<UUID>& boneEntityIds, Ref<Mesh> mesh);

		void UpdateAnimation(float dt, bool isRuntime);

	private:
		UUID mSceneID;
		entt::entity mSceneEntity = entt::null;
		entt::registry mRegistry;

		std::function<void(const std::string&)> mSceneTransitionCallback;

		std::string mName;
		bool mIsEditorScene = false;
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

		std::vector<std::function<void()>> mPostUpdateQueue;

		Ref<Renderer2D> mSceneRenderer2D;

		float mSkyboxLod = 1.0f;
		bool mIsPlaying = false;
		bool mShouldSimulate = false;

		friend class Entity;
		friend class Prefab;
		friend class Physics2D;
		friend class SceneRenderer;
		friend class SceneSerializer;
		friend class PrefabSerializer;
		friend class SceneHierarchyPanel;
		friend class ECSPanel;
	};
}