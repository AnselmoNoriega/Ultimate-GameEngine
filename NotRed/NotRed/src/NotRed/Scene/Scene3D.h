#pragma once

#include "NotRed/Core/UUID.h"

#include "NotRed/Renderer/Camera/Camera.h"
#include "NotRed/Renderer/Texture.h"
#include "NotRed/Renderer/Material.h"
#include "NotRed/Scene/SceneEnvironment.h"

#include "SceneCamera.h"

namespace NR
{

	struct Light
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

	struct LightEnvironment
	{
		DirectionalLight DirectionalLights[4];
	};

	class Entity;
	using EntityMap = std::unordered_map<UUID, Entity>;

	class Scene
	{
	public:
		Scene(const std::string& debugName = "Scene", bool isEditorScene = false);
		~Scene();

		void Init();

		void OnUpdate(float dt);
		void OnRenderRuntime(float dt);
		void OnRenderEditor(float dt, const EditorCamera& editorCamera);
		void OnEvent(Event& e);

		// Runtime
		void OnRuntimeStart();
		void OnRuntimeStop();

		void SetViewportSize(uint32_t width, uint32_t height);

		const Ref<Environment>& GetEnvironment() const { return mEnvironment; }
		void SetSkybox(const Ref<TextureCube>& skybox);

		Light& GetLight() { return mLight; }
		const Light& GetLight() const { return mLight; }

		Entity GetMainCameraEntity();

		float& GetSkyboxLod() { return mSkyboxLod; }
		float GetSkyboxLod() const { return mSkyboxLod; }

		Entity CreateEntity(const std::string& name = "");
		Entity CreateEntityWithID(UUID uuid, const std::string& name = "", bool runtimeMap = false);
		void DestroyEntity(Entity entity);

		void DuplicateEntity(Entity entity);

		template<typename T>
		auto GetAllEntitiesWith()
		{
			return mRegistry.view<T>();
		}

		Entity FindEntityByTag(const std::string& tag);
		Entity FindEntityByUUID(UUID id);

		glm::mat4 GetTransformRelativeToParent(Entity entity);

		const EntityMap& GetEntityMap() const { return mEntityIDMap; }
		void CopyTo(Ref<Scene>& target);

		UUID GetUUID() const { return mSceneID; }

		static Ref<Scene> GetScene(UUID uuid);

		float GetPhysics2DGravity() const;
		void SetPhysics2DGravity(float gravity);

		// Editor-specific
		void SetSelectedEntity(entt::entity entity) { mSelectedEntity = entity; }

	private:
		UUID mSceneID;
		entt::entity mSceneEntity;
		entt::registry mRegistry;

		std::string mDebugName;
		uint32_t mViewportWidth = 0, mViewportHeight = 0;

		EntityMap mEntityIDMap;

		Light mLight;
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

		friend class Entity;
		friend class SceneRenderer;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;

		friend void OnScriptComponentConstruct(entt::registry& registry, entt::entity entity);
		friend void OnScriptComponentDestroy(entt::registry& registry, entt::entity entity);
	};

}