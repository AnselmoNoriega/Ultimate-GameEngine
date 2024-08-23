#include "nrpch.h"
#include "Scene.h"

#include "Entity.h"
#include "Components.h"

#include "NotRed/Renderer/Renderer2D.h"
#include "NotRed/Renderer/SceneRenderer.h"
#include "NotRed/Script/ScriptEngine.h"

namespace NR
{
    std::unordered_map<uint32_t, Scene*> sActiveScenes;

    struct SceneComponent
    {
        uint32_t SceneID;
    };

    static uint32_t sSceneIDCounter = 0;

    void TransformConstruct(entt::registry& registry, entt::entity entity)
    {

    }

    void ScriptComponentConstruct(entt::registry& registry, entt::entity entity)
    {
        auto view = registry.view<SceneComponent>();
        uint32_t sceneID = 0;
        for (auto entity : view)
        {
            auto& scene = registry.get<SceneComponent>(entity);
            sceneID = scene.SceneID;
        }

        ScriptEngine::InitEntity(registry.get<ScriptComponent>(entity), (uint32_t)entity, sceneID);
    }

    Scene::Scene(const std::string& debugName)
        : mDebugName(debugName), mSceneID(++sSceneIDCounter)
    {
        mRegistry.on_construct<TransformComponent>().connect<&TransformConstruct>();
        mRegistry.on_construct<ScriptComponent>().connect<&ScriptComponentConstruct>();

        mSceneEntity = mRegistry.create();
        mRegistry.emplace<SceneComponent>(mSceneEntity, mSceneID);

        sActiveScenes[mSceneID] = this;

        Init();
    }

    Scene::~Scene()
    {
        mRegistry.clear();

        sActiveScenes.erase(mSceneID);
    }

    void Scene::Init()
    {
        auto skyboxShader = Shader::Create("Assets/Shaders/Skybox");
        mSkyboxMaterial = MaterialInstance::Create(Material::Create(skyboxShader));
        mSkyboxMaterial->ModifyFlags(MaterialFlag::DepthTest, false);
    }

    void Scene::Update(float dt)
    {
        Camera* camera = nullptr;
        {
            auto view = mRegistry.view<CameraComponent>();
            for (auto entity : view)
            {
                auto& comp = view.get<CameraComponent>(entity);
                camera = &comp.CameraObj;
                break;
            }
        }

        NR_CORE_ASSERT(camera, "Scene does not contain any cameras!");
        camera->Update(dt);

        {
            auto view = mRegistry.view<ScriptComponent>();
            for (auto entity : view)
            {
                ScriptEngine::UpdateEntity((uint32_t)entity, dt);
            }
        }

        mSkyboxMaterial->Set("uTextureLod", mSkyboxLod);

        auto entities = mRegistry.view<MeshComponent>();
        for (auto entity : entities)
        {
            auto& meshComponent = mRegistry.get<MeshComponent>(entity);
        }

        auto group = mRegistry.group<MeshComponent>(entt::get<TransformComponent>);
        SceneRenderer::BeginScene(this, *camera);
        for (auto entity : group)
        {
            auto [transformComponent, meshComponent] = group.get<TransformComponent, MeshComponent>(entity);
            if (meshComponent.MeshObj)
            {
                meshComponent.MeshObj->Update(dt);

                SceneRenderer::SubmitMesh(meshComponent, transformComponent, nullptr);
            }
        }

        SceneRenderer::EndScene();
    }

    void Scene::OnEvent(Event& e)
    {
        auto view = mRegistry.view<CameraComponent>();
        for (auto entity : view)
        {
            auto& comp = view.get<CameraComponent>(entity);
            comp.CameraObj.OnEvent(e);
            break;
        }
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

    Entity Scene::CreateEntity(const std::string& name)
    {
        auto entity = Entity{ mRegistry.create(), this };
        entity.AddComponent<TransformComponent>(glm::mat4(1.0f));
        entity.AddComponent<TagComponent>(name);
        return entity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        mRegistry.destroy(entity.mEntityHandle);
    }

    Environment Environment::Load(const std::string& filepath)
    {
        auto [radiance, irradiance] = SceneRenderer::CreateEnvironmentMap(filepath);
        return { radiance, irradiance };
    }
}