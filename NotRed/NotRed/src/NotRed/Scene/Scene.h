#pragma once

#include "NotRed/Renderer/Camera.h"
#include "NotRed/Renderer/Texture.h"
#include "NotRed/Renderer/Material.h"

#include "entt.hpp"

namespace NR
{
	struct Environment
	{
		Ref<TextureCube> RadianceMap;
		Ref<TextureCube> IrradianceMap;

		static Environment Load(const std::string& filepath);
	};

	struct Light
	{
		glm::vec3 Direction;
		glm::vec3 Radiance;
		float Multiplier = 1.0f;
	};

	class Entity;

	class Scene : public RefCounted
	{
	public:
		Scene(const std::string& debugName = "Scene");
		~Scene();

		void Init();

		void Update(float dt);
		void OnEvent(Event& e);

		void SetEnvironment(const Environment& environment);
		void SetSkybox(const Ref<TextureCube>& skybox);

		float& GetSkyboxLod() { return mSkyboxLod; }

		Light& GetLight() { return mLight; }

		Entity CreateEntity(const std::string& name = "Entity");
		void DestroyEntity(Entity entity);

		template<typename T>
		auto GetAllEntitiesWith()
		{
			return mRegistry.view<T>();
		}

	private:
		uint32_t mSceneID;
		entt::entity mSceneEntity;
		entt::registry mRegistry;

		std::string mDebugName;

		Light mLight;
		float mLightMultiplier = 0.3f;

		Environment mEnvironment;
		Ref<TextureCube> mSkyboxTexture;
		Ref<MaterialInstance> mSkyboxMaterial;

		float mSkyboxLod = 1.0f;

		friend class Entity;
		friend class SceneRenderer;
		friend class SceneHierarchyPanel;
	};
}