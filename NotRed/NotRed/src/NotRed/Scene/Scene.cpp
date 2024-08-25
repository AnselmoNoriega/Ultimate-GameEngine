#include "nrpch.h"
#include "Scene.h"

#include "Entity.h"
#include "Components.h"

#include "NotRed/Renderer/Renderer2D.h"
#include "NotRed/Renderer/SceneRenderer.h"
#include "NotRed/Script/ScriptEngine.h"

namespace NR
{
    std::unordered_map<UUID, Scene*> sActiveScenes;

    struct SceneComponent
    {
        UUID SceneID;
    };

    void ScriptComponentConstruct(entt::registry& registry, entt::entity entity)
    {
        auto sceneView = registry.view<SceneComponent>();
        UUID sceneID = registry.get<SceneComponent>(sceneView.front()).SceneID;
        Scene* scene = sActiveScenes[sceneID];

        auto entityID = registry.get<IDComponent>(entity).ID;
        NR_CORE_ASSERT(scene->mEntityIDMap.find(entityID) != scene->mEntityIDMap.end());
        ScriptEngine::InitScriptEntity(scene->mEntityIDMap.at(entityID));
    }

    Scene::Scene(const std::string& debugName)
        : mDebugName(debugName)
    {
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

        ScriptEngine::SceneDestruct(mSceneID);
    }

    void Scene::Init()
    {
        auto skyboxShader = Shader::Create("Assets/Shaders/Skybox");
        mSkyboxMaterial = MaterialInstance::Create(Material::Create(skyboxShader));
        mSkyboxMaterial->ModifyFlags(MaterialFlag::DepthTest, false);
    }

    void Scene::Update(float dt)
    {
        {
            auto view = mRegistry.view<ScriptComponent>();
            for (auto entity : view)
            {
                UUID entityID = mRegistry.get<IDComponent>(entity).ID;
                Entity e = { entity, this };
                if (ScriptEngine::ModuleExists(e.GetComponent<ScriptComponent>().ModuleName))
                {
                    ScriptEngine::UpdateEntity(mSceneID, entityID, dt);
                }
            }
        }
    }

    void Scene::RenderRuntime(float dt)
    {
        Entity cameraEntity = GetMainCameraEntity();
        if (!cameraEntity)
        {
            return;
        }

        glm::mat4 cameraViewMatrix = glm::inverse(cameraEntity.GetComponent<TransformComponent>().Transform);
        NR_CORE_ASSERT(cameraEntity, "Scene does not contain any cameras!");
        SceneCamera& camera = cameraEntity.GetComponent<CameraComponent>();
        camera.SetViewportSize(mViewportWidth, mViewportHeight);

        {
            mSkyboxMaterial->Set("uTextureLod", mSkyboxLod);

            auto group = mRegistry.group<MeshComponent>(entt::get<TransformComponent>);
            SceneRenderer::BeginScene(this, { camera, cameraViewMatrix });
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

    }

    void Scene::RenderEditor(float dt, const EditorCamera& editorCamera)
    {
        mSkyboxMaterial->Set("uTextureLod", mSkyboxLod);

        auto group = mRegistry.group<MeshComponent>(entt::get<TransformComponent>);
        SceneRenderer::BeginScene(this, { editorCamera, editorCamera.GetViewMatrix() });
        for (auto entity : group)
        {
            auto [transformComponent, meshComponent] = group.get<TransformComponent, MeshComponent>(entity);
            if (meshComponent.MeshObj)
            {
                meshComponent.MeshObj->Update(dt);

                if (mSelectedEntity == entity)
                {
                    SceneRenderer::SubmitSelectedMesh(meshComponent, transformComponent);
                }
                else
                {
                    SceneRenderer::SubmitMesh(meshComponent, transformComponent, nullptr);
                }
            }
        }

        SceneRenderer::EndScene();
    }

    void Scene::OnEvent(Event& e)
    {
    }

    void Scene::RuntimeStart()
    {
        ScriptEngine::SetSceneContext(this);

        auto view = mRegistry.view<ScriptComponent>();
        for (auto entity : view)
        {
            Entity e = { entity, this };
            if (ScriptEngine::ModuleExists(e.GetComponent<ScriptComponent>().ModuleName))
            {
                ScriptEngine::InstantiateEntityClass(e);
            }
        }

        mIsPlaying = true;
    }

    void Scene::RuntimeStop()
    {
        mIsPlaying = false;
    }

    void Scene::SetViewportSize(uint32_t width, uint32_t height)
    {
        mViewportWidth = width;
        mViewportHeight = height;
    }

    void Scene::SetEnvironment(const Environment& environment)
    {
        mEnvironment = environment;
        SetSkybox(environment.RadianceMap);
    }

    Entity Scene::GetMainCameraEntity()
    {
        auto view = mRegistry.view<CameraComponent>();
        for (auto entity : view)
        {
            auto& comp = view.get<CameraComponent>(entity);
            if (comp.Primary)
            {
                return { entity, this };
            }
        }
        return {};
    }

    void Scene::SetSkybox(const Ref<TextureCube>& skybox)
    {
        mSkyboxTexture = skybox;
        mSkyboxMaterial->Set("uTexture", skybox);
    }

    Entity Scene::CreateEntity(const std::string& name)
    {
        auto entity = Entity{ mRegistry.create(), this };
        auto& idComponent = entity.AddComponent<IDComponent>();
        idComponent.ID = {};

        entity.AddComponent<TransformComponent>(glm::mat4(1.0f));
        entity.AddComponent<TagComponent>(name);

        mEntityIDMap[idComponent.ID] = entity;
        return entity;
    }

    Entity Scene::CreateEntityWithID(UUID uuid, const std::string& name, bool runtimeMap)
    {
        auto entity = Entity{ mRegistry.create(), this };
        auto& idComponent = entity.AddComponent<IDComponent>();
        idComponent.ID = uuid;

        entity.AddComponent<TransformComponent>(glm::mat4(1.0f));
        entity.AddComponent<TagComponent>(name);

        NR_CORE_ASSERT(mEntityIDMap.find(uuid) == mEntityIDMap.end());
        mEntityIDMap[uuid] = entity;
        return entity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        if (entity.HasComponent<ScriptComponent>())
        {
            ScriptEngine::ScriptComponentDestroyed(mSceneID, entity.GetID());
        }
        mRegistry.destroy(entity.mEntityHandle);
    }

    template<typename T>
    static void CopyComponent(entt::registry& dstRegistry, entt::registry& srcRegistry, const std::unordered_map<UUID, entt::entity>& enttMap)
    {
        auto components = srcRegistry.view<T>();
        for (auto srcEntity : components)
        {
            entt::entity dstEntity = enttMap.at(srcRegistry.get<IDComponent>(srcEntity).ID);

            auto& srcComponent = srcRegistry.get<T>(srcEntity);
            auto& destComponent = dstRegistry.emplace_or_replace<T>(dstEntity, srcComponent);
        }
    }

    template<typename T>
    static void CopyComponentIfExists(entt::entity dst, entt::entity src, entt::registry& registry)
    {
        if (registry.all_of<T>(src))
        {
            auto& srcComponent = registry.get<T>(src);
            registry.emplace_or_replace<T>(dst, srcComponent);
        }
    }

    void Scene::DuplicateEntity(Entity entity)
    {
        Entity newEntity;
        newEntity = CreateEntity(entity.GetComponent<TagComponent>().Tag);

        CopyComponentIfExists<TransformComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<MeshComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<ScriptComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<CameraComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<SpriteRendererComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
    }

    // Copy to runtime
    void Scene::CopyTo(Ref<Scene>& target)
    {
        target->mLight = mLight;
        target->mLightMultiplier = mLightMultiplier;

        target->mEnvironment = mEnvironment;
        target->mSkyboxTexture = mSkyboxTexture;
        target->mSkyboxMaterial = mSkyboxMaterial;
        target->mSkyboxLod = mSkyboxLod;

        std::unordered_map<UUID, entt::entity> enttMap;
        auto idComponents = mRegistry.view<IDComponent>();
        for (auto entity : idComponents)
        {
            auto uuid = mRegistry.get<IDComponent>(entity).ID;
            Entity e = target->CreateEntityWithID(uuid, "", true);
            enttMap[uuid] = e.mEntityHandle;
        }

        CopyComponent<TagComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<TransformComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<MeshComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<ScriptComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<CameraComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<SpriteRendererComponent>(target->mRegistry, mRegistry, enttMap);

        const auto& entityInstanceMap = ScriptEngine::GetEntityInstanceMap();
        if (entityInstanceMap.find(target->GetID()) != entityInstanceMap.end())
        {
            ScriptEngine::CopyEntityScriptData(target->GetID(), mSceneID);
        }
    }

    Ref<Scene> Scene::GetScene(UUID uuid)
    {
        if (sActiveScenes.find(uuid) != sActiveScenes.end())
        {
            return sActiveScenes.at(uuid);
        }

        return {};
    }


    Environment Environment::Load(const std::string& filepath)
    {
        auto [radiance, irradiance] = SceneRenderer::CreateEnvironmentMap(filepath);
        return { filepath, radiance, irradiance };
    }
}