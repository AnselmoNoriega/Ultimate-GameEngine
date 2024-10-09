#pragma once

#include "NotRed/Core/UUID.h"
#include "NotRed/Core/TimeFrame.h"

#include "NotRed/Renderer/Camera.h"
#include "NotRed/Renderer/Texture.h"
#include "NotRed/Renderer/Material.h"
#include "NotRed/Renderer/SceneEnvironment.h"

#include "SceneCamera.h"
#include "NotRed/Editor/EditorCamera.h"

#include "entt.hpp"

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

		bool CastShadows = true;
	};

	struct PointLight
	{
		glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
		float Multiplier = 0.0f;
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };
		float NearPlane = 0.001f;
		float FarPlane = 25.0f;
		bool CastsShadows = true;
		glm::vec2 Padding{};
	};

	struct LightEnvironment
	{
		DirectionalLight DirectionalLights[4];
		std::vector<PointLight> PointLights;
	};

	class Entity;
	struct TransformComponent;

	using EntityMap = std::unordered_map<UUID, Entity>;

	class Scene : public RefCounted
	{
	public:
		Scene(const std::string& debugName = "Scene", bool isEditorScene = false);
		~Scene();

		void Init();

		void Update(float dt);
		void RenderRuntime(float dt);
		void RenderEditor(float dt, const EditorCamera& editorCamera);
		void RenderSimulation(float dt, const EditorCamera& editorCamera);
		void OnEvent(Event& e);

		void RuntimeStart();
		void RuntimeStop();

		void SimulationStart();
		void SimulationEnd();

		void SetViewportSize(uint32_t width, uint32_t height);

		const Ref<Environment>& GetEnvironment() const { return mEnvironment; }
		void SetSkybox(const Ref<TextureCube>& skybox);

		float& GetSkyboxLod() { return mSkyboxLod; }
		float GetSkyboxLod() const { return mSkyboxLod; }

		Light& GetLight() { return mLight; }
		const Light& GetLight() const { return mLight; }

		Entity CreateEntity(const std::string& name = "Entity");
		Entity CreateEntityWithID(UUID uuid, const std::string& name = "Entity", bool runtimeMap = false);
		void DuplicateEntity(Entity entity);
		void DestroyEntity(Entity entity);

		Entity GetMainCameraEntity();
		Entity FindEntityByTag(const std::string& tag);
		Entity FindEntityByID(UUID id);

		void ConvertToLocalSpace(Entity entity);
		void ConvertToWorldSpace(Entity entity);

		glm::mat4 GetTransformRelativeToParent(Entity entity);
		glm::mat4 GetWorldSpaceTransformMatrix(Entity entity);
		TransformComponent GetWorldSpaceTransform(Entity entity);

		void ParentEntity(Entity entity, Entity parent);
		void UnparentEntity(Entity entity);

		template<typename T>
		auto GetAllEntitiesWith()
		{
			return mRegistry.view<T>();
		}

		const EntityMap& GetEntityMap() const { return mEntityIDMap; }
		void CopyTo(Ref<Scene>& target);

		UUID GetID() const { return mSceneID; }

		static Ref<Scene> GetScene(UUID uuid);

		bool IsEditorScene() const { return mIsEditorScene; }
		bool IsPlaying() const { return mIsPlaying; }

		float GetPhysics2DGravity() const;
		void SetPhysics2DGravity(float gravity);

		void SetSelectedEntity(entt::entity entity) { mSelectedEntity = entity; }

	private:
		UUID mSceneID;
		entt::entity mSceneEntity;
		entt::registry mRegistry;

		std::string mDebugName;
		bool mIsEditorScene = false;
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
		bool mShouldSimulate = false;

		friend class Entity;
		friend class SceneRenderer;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;

		friend void ScriptComponentConstruct(entt::registry& registry, entt::entity entity);
		friend void ScriptComponentDestroy(entt::registry& registry, entt::entity entity);
		friend void AudioComponentConstruct(entt::registry& registry, entt::entity entity);
		friend void AudioComponentDestroy(entt::registry& registry, entt::entity entity);
	};
}