#pragma once

#include "Entity.h"
#include "NotRed/Renderer/Camera.h"

namespace NR
{
	struct Environment
	{
		Ref<TextureCube> RadianceMap;
		Ref<TextureCube> IrradianceMap;

		static Environment Load(const std::string& filepath);
	};

	class Scene
	{
	public:
		Scene(const std::string& debugName = "Scene");
		~Scene();

		void Init();

		void Update(float dt);
		void OnEvent(Event& e);

		void SetCamera(const Camera& camera);
		Camera& GetCamera() { return mCamera; }

		void SetEnvironment(const Environment& environment);
		void SetSkybox(const Ref<TextureCube>& skybox);

		float& GetSkyboxLod() { return mSkyboxLod; }

		void AddEntity(Entity* entity);
		Entity* CreateEntity(const std::string& name = "Entity");

	private:
		std::string mDebugName;
		std::vector<Entity*> mEntities;
		Camera mCamera;

		Environment mEnvironment;
		Ref<TextureCube> mSkyboxTexture;
		Ref<MaterialInstance> mSkyboxMaterial;

		float mSkyboxLod = 1.0f;

		friend class SceneRenderer;
		friend class SceneHierarchyPanel;
	};
}