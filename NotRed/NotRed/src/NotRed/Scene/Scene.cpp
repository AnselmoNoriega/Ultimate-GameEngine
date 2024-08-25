#include "nrpch.h"
#include "Scene.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <box2d/box2d.h>

#include "NotRed/Core/Input.h"

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

    struct Box2DWorldComponent
    {
        std::unique_ptr<b2World> World;
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

        mRegistry.emplace<Box2DWorldComponent>(mSceneEntity, std::make_unique<b2World>(b2Vec2{ 0.0f, -9.8f }));

        sActiveScenes[mSceneID] = this;

        Init();
        SetEnvironment(Environment::Load("Assets/Env/HDR_blue_nebulae-1.hdr"));
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

    static std::tuple<glm::vec3, glm::quat, glm::vec3> GetTransformDecomposition(const glm::mat4& transform)
    {
        glm::vec3 scale, translation, skew;
        glm::vec4 perspective;
        glm::quat orientation;
        glm::decompose(transform, scale, orientation, translation, skew, perspective);

        return { translation, orientation, scale };
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
        {
            auto sceneView = mRegistry.view<Box2DWorldComponent>();
            auto& box2DWorld = mRegistry.get<Box2DWorldComponent>(sceneView.front()).World;
            int32_t velocityIterations = 6;
            int32_t positionIterations = 2;
            box2DWorld->Step(dt, velocityIterations, positionIterations);

            auto view = mRegistry.view<RigidBody2DComponent>();
            for (auto entity : view)
            {
                Entity e = { entity, this };
                auto& transform = e.Transform();
                auto& rb2d = e.GetComponent<RigidBody2DComponent>();
                b2Body* body = static_cast<b2Body*>(rb2d.RuntimeBody);

                auto& position = body->GetPosition();
                auto [translation, rotationQuat, scale] = GetTransformDecomposition(transform);
                glm::vec3 rotation = glm::eulerAngles(rotationQuat);

                transform = glm::translate(glm::mat4(1.0f), { position.x, position.y, transform[3].z }) *
                    glm::toMat4(glm::quat({ rotation.x, rotation.y, body->GetAngle() })) *
                    glm::scale(glm::mat4(1.0f), scale);
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

        {
            auto view = mRegistry.view<ScriptComponent>();
            for (auto entity : view)
            {
                Entity e = { entity, this };
                if (ScriptEngine::ModuleExists(e.GetComponent<ScriptComponent>().ModuleName))
                    ScriptEngine::InstantiateEntityClass(e);
            }
        }

        // Box2D physics
        auto sceneView = mRegistry.view<Box2DWorldComponent>();
        auto& world = mRegistry.get<Box2DWorldComponent>(sceneView.front()).World;
        {
            auto view = mRegistry.view<RigidBody2DComponent>();
            for (auto entity : view)
            {
                Entity e = { entity, this };
                auto& transform = e.Transform();
                auto& rigidBody2D = mRegistry.get<RigidBody2DComponent>(entity);

                b2BodyDef bodyDef;
                if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Static)
                {
                    bodyDef.type = b2_staticBody;
                }
                else if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Dynamic)
                {
                    bodyDef.type = b2_dynamicBody;
                }
                else if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Kinematic)
                {
                    bodyDef.type = b2_kinematicBody;
                }
                bodyDef.position.Set(transform[3].x, transform[3].y);

                auto [translation, rotationQuat, scale] = GetTransformDecomposition(transform);
                glm::vec3 rotation = glm::eulerAngles(rotationQuat);
                bodyDef.angle = rotation.z;
                rigidBody2D.RuntimeBody = world->CreateBody(&bodyDef);
            }
        }

        {
            auto view = mRegistry.view<BoxCollider2DComponent>();
            for (auto entity : view)
            {
                Entity e = { entity, this };
                auto& transform = e.Transform();

                auto& boxCollider2D = mRegistry.get<BoxCollider2DComponent>(entity);
                if (e.HasComponent<RigidBody2DComponent>())
                {
                    auto& rigidBody2D = e.GetComponent<RigidBody2DComponent>();
                    NR_CORE_ASSERT(rigidBody2D.RuntimeBody);
                    b2Body* body = static_cast<b2Body*>(rigidBody2D.RuntimeBody);

                    b2PolygonShape polygonShape;
                    polygonShape.SetAsBox(boxCollider2D.Size.x, boxCollider2D.Size.y);

                    b2FixtureDef fixtureDef;
                    fixtureDef.shape = &polygonShape;
                    fixtureDef.density = 1.0f;
                    fixtureDef.friction = 1.0f;
                    body->CreateFixture(&fixtureDef);
                }
            }
        }

        {
            auto view = mRegistry.view<CircleCollider2DComponent>();
            for (auto entity : view)
            {
                Entity e = { entity, this };
                auto& transform = e.Transform();

                auto& circleCollider2D = mRegistry.get<CircleCollider2DComponent>(entity);
                if (e.HasComponent<RigidBody2DComponent>())
                {
                    auto& rigidBody2D = e.GetComponent<RigidBody2DComponent>();
                    NR_CORE_ASSERT(rigidBody2D.RuntimeBody);
                    b2Body* body = static_cast<b2Body*>(rigidBody2D.RuntimeBody);

                    b2CircleShape circleShape;
                    circleShape.m_radius = circleCollider2D.Radius;

                    b2FixtureDef fixtureDef;
                    fixtureDef.shape = &circleShape;
                    fixtureDef.density = 1.0f;
                    fixtureDef.friction = 1.0f;
                    body->CreateFixture(&fixtureDef);
                }
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
        CopyComponentIfExists<RigidBody2DComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<BoxCollider2DComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<CircleCollider2DComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
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
        CopyComponent<RigidBody2DComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<BoxCollider2DComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<CircleCollider2DComponent>(target->mRegistry, mRegistry, enttMap);

        const auto& entityInstanceMap = ScriptEngine::GetEntityInstanceMap();
        if (entityInstanceMap.find(target->GetID()) != entityInstanceMap.end())
        {
            ScriptEngine::CopyEntityScriptData(target->GetID(), mSceneID);
        }

        target->SetPhysics2DGravity(GetPhysics2DGravity());
    }

    Ref<Scene> Scene::GetScene(UUID uuid)
    {
        if (sActiveScenes.find(uuid) != sActiveScenes.end())
        {
            return sActiveScenes.at(uuid);
        }

        return {};
    }

    float Scene::GetPhysics2DGravity() const
    {
        return mRegistry.get<Box2DWorldComponent>(mSceneEntity).World->GetGravity().y;
    }

    void Scene::SetPhysics2DGravity(float gravity)
    {
        mRegistry.get<Box2DWorldComponent>(mSceneEntity).World->SetGravity({ 0.0f, gravity });
    }


    Environment Environment::Load(const std::string& filepath)
    {
        auto [radiance, irradiance] = SceneRenderer::CreateEnvironmentMap(filepath);
        return { filepath, radiance, irradiance };
    }
}