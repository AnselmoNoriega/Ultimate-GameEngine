#include "nrpch.h"
#include "Scene.h"

#include "NotRed/Renderer/SceneRenderer.h"

namespace NR
{
	Scene::Scene(const std::string& debugName)
		: mDebugName(debugName)
	{
		Init();
	}

	Scene::~Scene()
	{
		for (Entity* entity : mEntities)
			delete entity;
	}

	void Scene::Init()
	{
		auto skyboxShader = Shader::Create("Assets/Shaders/Skybox");
		mSkyboxMaterial = MaterialInstance::Create(Material::Create(skyboxShader));
		mSkyboxMaterial->ModifyFlags(MaterialFlag::DepthTest, false);
	}

	void Scene::Update(float dt)
	{
		mCamera.Update(dt);

		mSkyboxMaterial->Set("uTextureLod", mSkyboxLod);

		for (auto entity : mEntities)
		{
			auto mesh = entity->GetMesh();
			if (mesh)
			{
				mesh->Update(dt);
			}
		}

		SceneRenderer::BeginScene(this);

		for (auto entity : mEntities)
		{
			SceneRenderer::SubmitEntity(entity);
		}

		SceneRenderer::EndScene();
	}

	void Scene::SetCamera(const Camera& camera)
	{
		mCamera = camera;
	}

	void Scene::SetEnvironment(const Environment& environment)
	{
		mEnvironment = environment;
		SetSkybox(environment.RadianceMap);
	}

	void Scene::SetSkybox(const Ref<TextureCube>& skybox)
	{
		mSkyboxTexture = skybox;
		mSkyboxMaterial->Set("uTexture", skybox);
	}

	void Scene::AddEntity(Entity* entity)
	{
		mEntities.push_back(entity);
	}

	Entity* Scene::CreateEntity()
	{
		Entity* entity = new Entity();
		AddEntity(entity);
		return entity;
	}

	Environment Environment::Load(const std::string& filepath)
	{
		auto [radiance, irradiance] = SceneRenderer::CreateEnvironmentMap(filepath);
		return { radiance, irradiance };
	}
}