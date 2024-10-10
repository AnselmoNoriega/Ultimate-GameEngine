#include "nrpch.h"
#include "Scene.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <box2d/box2d.h>

#include "Entity.h"
#include "Components.h"

#include "NotRed/Math/Math.h"
#include "NotRed/Renderer/Renderer.h"

#include "NotRed/Physics/PhysicsManager.h"
#include "NotRed/Renderer/Renderer2D.h"
#include "NotRed/Renderer/SceneRenderer.h"
#include "NotRed/Script/ScriptEngine.h"

#include "NotRed/Core/Input.h"

#include "NotRed/Audio/AudioEngine.h"
#include "NotRed/Audio/AudioComponent.h"

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

    class ContactListener2D : public b2ContactListener
    {
    public:
        void BeginContact(b2Contact* contact) override
        {
            Entity& a = *(Entity*)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
            Entity& b = *(Entity*)contact->GetFixtureB()->GetBody()->GetUserData().pointer;

            if (a.HasComponent<ScriptComponent>() && ScriptEngine::ModuleExists(a.GetComponent<ScriptComponent>().ModuleName))
            {
                ScriptEngine::Collision2DBegin(a);
            }

            if (b.HasComponent<ScriptComponent>() && ScriptEngine::ModuleExists(b.GetComponent<ScriptComponent>().ModuleName))
            {
                ScriptEngine::Collision2DBegin(b);
            }
        }

        /// Called when two fixtures cease to touch.
        void EndContact(b2Contact* contact) override
        {
            Entity& a = *(Entity*)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
            Entity& b = *(Entity*)contact->GetFixtureB()->GetBody()->GetUserData().pointer;

            if (a.HasComponent<ScriptComponent>() && ScriptEngine::ModuleExists(a.GetComponent<ScriptComponent>().ModuleName))
            {
                ScriptEngine::Collision2DEnd(a);
            }

            if (b.HasComponent<ScriptComponent>() && ScriptEngine::ModuleExists(b.GetComponent<ScriptComponent>().ModuleName))
            {
                ScriptEngine::Collision2DEnd(b);
            }
        }

        void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override
        {
            B2_NOT_USED(contact);
            B2_NOT_USED(oldManifold);
        }

        void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) override
        {
            B2_NOT_USED(contact);
            B2_NOT_USED(impulse);
        }
    };

    static ContactListener2D sBox2DContactListener;

    static void ScriptComponentConstruct(entt::registry& registry, entt::entity entity)
    {
        auto sceneView = registry.view<SceneComponent>();
        UUID sceneID = registry.get<SceneComponent>(sceneView.front()).SceneID;
        Scene* scene = sActiveScenes[sceneID];

        auto entityID = registry.get<IDComponent>(entity).ID;
        NR_CORE_ASSERT(scene->mEntityIDMap.find(entityID) != scene->mEntityIDMap.end());
        ScriptEngine::InitScriptEntity(scene->mEntityIDMap.at(entityID));
    }

    static void ScriptComponentDestroy(entt::registry& registry, entt::entity entity)
    {
        auto sceneView = registry.view<SceneComponent>();
        UUID sceneID = registry.get<SceneComponent>(sceneView.front()).SceneID;

        Scene* scene = sActiveScenes[sceneID];

        if (registry.all_of<IDComponent>(entity))
        {
            auto entityID = registry.get<IDComponent>(entity).ID;
            ScriptEngine::ScriptComponentDestroyed(sceneID, entityID);
        }
    }

    static void AudioComponentConstruct(entt::registry& registry, entt::entity entity)
    {
        auto sceneView = registry.view<SceneComponent>();
        UUID sceneID = registry.get<SceneComponent>(sceneView.front()).SceneID;
        Scene* scene = sActiveScenes[sceneID];
        auto entityID = registry.get<IDComponent>(entity).ID;

        NR_CORE_ASSERT(scene->mEntityIDMap.find(entityID) != scene->mEntityIDMap.end());
        registry.get<Audio::AudioComponent>(entity).ParentHandle = entityID;

        Audio::AudioEngine::Get().RegisterAudioComponent(scene->mEntityIDMap.at(entityID));
    }

    static void AudioComponentDestroy(entt::registry& registry, entt::entity entity)
    {
        auto sceneView = registry.view<SceneComponent>();
        UUID sceneID = registry.get<SceneComponent>(sceneView.front()).SceneID;
        Scene* scene = sActiveScenes[sceneID];
        auto entityID = registry.get<IDComponent>(entity).ID;

        NR_CORE_ASSERT(scene->mEntityIDMap.find(entityID) != scene->mEntityIDMap.end());

        Audio::AudioEngine::Get().UnregisterAudioComponent(sceneID, scene->mEntityIDMap.at(entityID).GetID());
    }

    Scene::Scene(const std::string& debugName, bool isEditorScene)
        : mDebugName(debugName), mIsEditorScene(isEditorScene)
    {
        mRegistry.on_construct<ScriptComponent>().connect<&ScriptComponentConstruct>();
        mRegistry.on_destroy<ScriptComponent>().connect<&ScriptComponentDestroy>();

        mRegistry.on_construct<Audio::AudioComponent>().connect<&AudioComponentConstruct>();

        mSceneEntity = mRegistry.create();
        mRegistry.emplace<SceneComponent>(mSceneEntity, mSceneID);

        Box2DWorldComponent& b2dWorld = mRegistry.emplace<Box2DWorldComponent>(mSceneEntity, std::make_unique<b2World>(b2Vec2{ 0.0f, -9.8f }));
        b2dWorld.World->SetContactListener(&sBox2DContactListener);

        sActiveScenes[mSceneID] = this;

        Init();
    }

    Scene::~Scene()
    {
        mRegistry.on_destroy<ScriptComponent>().disconnect();

        mRegistry.clear();
        sActiveScenes.erase(mSceneID);

        ScriptEngine::SceneDestruct(mSceneID);
        Audio::AudioEngine::SceneDestruct(mSceneID);
    }

    void Scene::Init()
    {
        auto skyboxShader = Renderer::GetShaderLibrary()->Get("Skybox");
        mSkyboxMaterial = Material::Create(skyboxShader);
        mSkyboxMaterial->ModifyFlags(MaterialFlag::DepthTest, false);

        Renderer::GenerateParticles();
    }

    void Scene::Update(float dt)
    {
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
                auto& rb2d = e.GetComponent<RigidBody2DComponent>();
                b2Body* body = static_cast<b2Body*>(rb2d.RuntimeBody);

                auto& position = body->GetPosition();
                auto& transform = e.GetComponent<TransformComponent>();

                transform.Translation.x = position.x;
                transform.Translation.y = position.y;
                transform.Rotation.z = glm::degrees(body->GetAngle());
            }
        }

        if (mIsPlaying && mShouldSimulate)
        {
            auto view = mRegistry.view<ScriptComponent>();
            for (auto entity : view)
            {
                UUID entityID = mRegistry.get<IDComponent>(entity).ID;
                Entity e = { entity, this };
                if (ScriptEngine::ModuleExists(e.GetComponent<ScriptComponent>().ModuleName))
                {
                    ScriptEngine::UpdateEntity(e, dt);
                }
            }
        }
        {
            auto view = mRegistry.view<TransformComponent>();
            for (auto entity : view)
            {
                auto& transformComponent = view.get<TransformComponent>(entity);
                Entity e = Entity(entity, this);
                glm::mat4 transform = GetTransformRelativeToParent(e);
                glm::vec3 translation;
                glm::vec3 rotation;
                glm::vec3 scale;
                Math::DecomposeTransform(transform, translation, rotation, scale);

                glm::quat rotationQuat = glm::quat(rotation);
                transformComponent.Up = glm::normalize(glm::rotate(rotationQuat, glm::vec3(0.0f, 1.0f, 0.0f)));
                transformComponent.Right = glm::normalize(glm::rotate(rotationQuat, glm::vec3(1.0f, 0.0f, 0.0f)));
                transformComponent.Forward = glm::normalize(glm::rotate(rotationQuat, glm::vec3(0.0f, 0.0f, -1.0f)));
            }
        }
        {
            auto view = mRegistry.view<AudioListenerComponent>();
            Entity listener;
            for (auto entity : view)
            {
                Entity e = { entity, this };
                if (e.GetComponent<AudioListenerComponent>().Active)
                {
                    listener = e;
                    auto worldSpaceTransform = GetWorldSpaceTransform(listener);
                    Audio::AudioEngine::Get().UpdateListenerPosition(worldSpaceTransform.Translation, worldSpaceTransform.Forward);

                    if (auto physicsActor = PhysicsManager::GetScene()->GetActor(listener))
                    {
                        if (physicsActor->IsDynamic())
                        {
                            Audio::AudioEngine::Get().UpdateListenerVelocity(physicsActor->GetVelocity());
                        }
                    }
                    break;
                }
            }

            if (listener.mEntityHandle == entt::null)
            {
                listener = GetMainCameraEntity();
                if (listener.mEntityHandle != entt::null)
                {
                    if (!listener.HasComponent<AudioListenerComponent>())
                    {
                        listener.AddComponent<AudioListenerComponent>();
                    }

                    auto worldSpaceTransform = GetWorldSpaceTransform(listener);
                    Audio::AudioEngine::Get().UpdateListenerPosition(worldSpaceTransform.Translation, worldSpaceTransform.Forward);

                    if (auto physicsActor = PhysicsManager::GetScene()->GetActor(listener))
                    {
                        if (physicsActor->IsDynamic())
                        {
                            Audio::AudioEngine::Get().UpdateListenerVelocity(physicsActor->GetVelocity());
                        }
                    }
                }
            }
        }
        {
            auto view = mRegistry.view<Audio::AudioComponent>();

            std::vector<Entity> deadEntities;
            deadEntities.reserve(view.size());

            std::vector<Audio::SoundSourceUpdateData> updateData;
            updateData.reserve(view.size());

            for (auto entity : view)
            {
                Entity e = { entity, this };
                auto& audioComponent = e.GetComponent<Audio::AudioComponent>();

                if (audioComponent.AutoDestroy && audioComponent.MarkedForDestroy)
                {
                    deadEntities.push_back(e);
                    continue;
                }
                const auto& worldSpaceTransform = GetWorldSpaceTransform(e);

                glm::vec3 velocity{ 0.0f, 0.0f, 0.0f };
                if (const auto& physicsActor = PhysicsManager::GetScene()->GetActor(e))
                {
                    if (physicsActor->IsDynamic())
                    {
                        velocity = physicsActor->GetVelocity();
                    }
                }

                updateData.emplace_back(Audio::SoundSourceUpdateData{
                    e.GetID(),
                    audioComponent.VolumeMultiplier,
                    audioComponent.PitchMultiplier,
                    worldSpaceTransform.Translation,
                    velocity });
            }

            Audio::AudioEngine::Get().SubmitSourceUpdateData(updateData);
            for (int i = deadEntities.size() - 1; i >= 0; --i)
            {
                DestroyEntity(deadEntities[i]);
            }
        }

        if (mShouldSimulate)
        {
            PhysicsManager::GetScene()->Simulate(dt, mIsPlaying);
        }
    }

    void Scene::RenderRuntime(Ref<SceneRenderer> renderer, float dt)
    {
        Entity cameraEntity = GetMainCameraEntity();
        if (!cameraEntity)
        {
            return;
        }

        glm::mat4 cameraViewMatrix = glm::inverse(GetTransformRelativeToParent(cameraEntity));
        NR_CORE_ASSERT(cameraEntity, "Scene does not contain any cameras!");
        SceneCamera& camera = cameraEntity.GetComponent<CameraComponent>();
        camera.SetViewportSize(mViewportWidth, mViewportHeight);

        // Process lights
        {
            mLightEnvironment = LightEnvironment();
            // Directional Lights	
            {
                auto lights = mRegistry.group<DirectionalLightComponent>(entt::get<TransformComponent>);
                uint32_t directionalLightIndex = 0;
                for (auto entity : lights)
                {
                    auto [transformComponent, lightComponent] = lights.get<TransformComponent, DirectionalLightComponent>(entity);
                    glm::vec3 direction = -glm::normalize(glm::mat3(transformComponent.GetTransform()) * glm::vec3(1.0f));
                    mLightEnvironment.DirectionalLights[directionalLightIndex++] =
                    {
                        direction,
                        lightComponent.Radiance,
                        lightComponent.Intensity,
                        lightComponent.CastShadows
                    };
                }
                //Point Light
                {
                    auto pointLights = mRegistry.group<PointLightComponent>(entt::get<TransformComponent>);
                    mLightEnvironment.PointLights.resize(pointLights.size());
                    uint32_t pointLightIndex = 0;
                    for (auto entity : pointLights)
                    {
                        auto [transformComponent, lightComponent] = pointLights.get<TransformComponent, PointLightComponent>(entity);
                        mLightEnvironment.PointLights[pointLightIndex++] = {
                            transformComponent.Translation,
                            lightComponent.Intensity,
                            lightComponent.Radiance,
                            lightComponent.NearPlane,
                            lightComponent.FarPlane,
                            lightComponent.CastsShadows,
                        };
                    }
                }
            }
        }

        {
            auto lights = mRegistry.group<SkyLightComponent>(entt::get<TransformComponent>);
            if (lights.empty())
            {
                mEnvironment = Ref<Environment>::Create(Renderer::GetBlackCubeTexture(), Renderer::GetBlackCubeTexture());
            }

            for (auto entity : lights)
            {
                auto [transformComponent, skyLightComponent] = lights.get<TransformComponent, SkyLightComponent>(entity);
                if (!skyLightComponent.SceneEnvironment && skyLightComponent.DynamicSky)
                {
                    Ref<TextureCube> preethamEnv = Renderer::CreatePreethamSky(skyLightComponent.TurbidityAzimuthInclination.x, skyLightComponent.TurbidityAzimuthInclination.y, skyLightComponent.TurbidityAzimuthInclination.z);
                    skyLightComponent.SceneEnvironment = Ref<Environment>::Create(preethamEnv, preethamEnv);
                }
                mEnvironment = skyLightComponent.SceneEnvironment;
                mEnvironmentIntensity = skyLightComponent.Intensity;
                if (mEnvironment)
                {
                    SetSkybox(mEnvironment->RadianceMap);
                }
            }
        }

        {
            mSkyboxMaterial->Set("uUniforms.TextureLod", mSkyboxLod);

            auto group = mRegistry.group<MeshComponent>(entt::get<TransformComponent>);

            renderer->SetScene(this);
            renderer->BeginScene({ camera, cameraViewMatrix });

            for (auto entity : group)
            {
                auto [transformComponent, meshComponent] = group.get<TransformComponent, MeshComponent>(entity);
                if (meshComponent.MeshObj && meshComponent.MeshObj->Type == AssetType::Mesh)
                {
                    Entity e = Entity(entity, this);

                    meshComponent.MeshObj->Update(dt);
                    glm::mat4 transform = GetTransformRelativeToParent(e);

                    if (e.HasComponent<RigidBodyComponent>())
                    {
                        transform = e.Transform().GetTransform();
                    }

                    renderer->SubmitMesh(meshComponent, transform);
                }
            }

            auto groupParticles = mRegistry.group<ParticleComponent>(entt::get<TransformComponent>);
            for (auto entity : groupParticles)
            {
                auto [transformComponent, particleComponent] = groupParticles.get<TransformComponent, ParticleComponent>(entity);

                glm::mat4 transform = GetTransformRelativeToParent(Entity(entity, this));
                renderer->SubmitParticles(particleComponent, transform, particleComponent.MeshObj->GetMaterials()[0]);
            }

            renderer->EndScene();
        }
    }

    void Scene::RenderEditor(Ref<SceneRenderer> renderer, float dt, const EditorCamera& editorCamera)
    {
        // Process lights
        {
            mLightEnvironment = LightEnvironment();
            //Directional Lights
            {
                auto dirLights = mRegistry.group<DirectionalLightComponent>(entt::get<TransformComponent>);
                uint32_t directionalLightIndex = 0;
                for (auto entity : dirLights)
                {
                    auto [transformComponent, lightComponent] = dirLights.get<TransformComponent, DirectionalLightComponent>(entity);
                    glm::vec3 direction = -glm::normalize(glm::mat3(transformComponent.GetTransform()) * glm::vec3(1.0f));
                    mLightEnvironment.DirectionalLights[directionalLightIndex++] =
                    {
                        direction,
                        lightComponent.Radiance,
                        lightComponent.Intensity,
                        lightComponent.CastShadows
                    };
                }
            }
            //Point Lights
            {
                auto pointLights = mRegistry.group<PointLightComponent>(entt::get<TransformComponent>);
                mLightEnvironment.PointLights.resize(pointLights.size());
                uint32_t pointLightIndex = 0;
                for (auto entity : pointLights)
                {
                    auto [transformComponent, lightComponent] = pointLights.get<TransformComponent, PointLightComponent>(entity);
                    mLightEnvironment.PointLights[pointLightIndex++] = {
                        transformComponent.Translation,
                        lightComponent.Intensity,
                        lightComponent.Radiance,
                        lightComponent.NearPlane,
                        lightComponent.FarPlane,
                        lightComponent.CastsShadows,
                    };
                }
            }
        }

        {
            auto lights = mRegistry.group<SkyLightComponent>(entt::get<TransformComponent>);
            if (lights.empty())
            {
                mEnvironment = Ref<Environment>::Create(Renderer::GetBlackCubeTexture(), Renderer::GetBlackCubeTexture());
            }
            for (auto entity : lights)
            {
                auto [transformComponent, skyLightComponent] = lights.get<TransformComponent, SkyLightComponent>(entity);
                if (!skyLightComponent.SceneEnvironment && skyLightComponent.DynamicSky)
                {
                    Ref<TextureCube> preethamEnv = Renderer::CreatePreethamSky(skyLightComponent.TurbidityAzimuthInclination.x, skyLightComponent.TurbidityAzimuthInclination.y, skyLightComponent.TurbidityAzimuthInclination.z);
                    skyLightComponent.SceneEnvironment = Ref<Environment>::Create(preethamEnv, preethamEnv);
                }
                mEnvironment = skyLightComponent.SceneEnvironment;
                mEnvironmentIntensity = skyLightComponent.Intensity;
                if (mEnvironment)
                {
                    SetSkybox(mEnvironment->RadianceMap);
                }
            }
        }

        mSkyboxMaterial->Set("uUniforms.TextureLod", mSkyboxLod);

        auto group = mRegistry.group<MeshComponent>(entt::get<TransformComponent>);

        renderer->SetScene(this);
        renderer->BeginScene({ editorCamera, editorCamera.GetViewMatrix(), 0.1f, 1000.0f, 45.0f });

        for (auto entity : group)
        {
            auto&& [meshComponent, transformComponent] = group.get<MeshComponent, TransformComponent>(entity);
            if (meshComponent.MeshObj && meshComponent.MeshObj->Type == AssetType::Mesh)
            {
                meshComponent.MeshObj->Update(dt);

                glm::mat4 transform = GetTransformRelativeToParent(Entity{ entity, this });

                if (mSelectedEntity == entity)
                {
                    renderer->SubmitSelectedMesh(meshComponent, transform);
                }
                else
                {
                    renderer->SubmitMesh(meshComponent, transform);
                }
            }
        }

        auto groupParticle = mRegistry.group<ParticleComponent>(entt::get<TransformComponent>);
        for (auto entity : groupParticle)
        {
            auto& particleComponent = groupParticle.get<ParticleComponent>(entity);
            auto& transformComponent = groupParticle.get<TransformComponent>(entity);
            particleComponent.MeshObj->Update(dt);

            glm::mat4 transform = GetTransformRelativeToParent(Entity{ entity, this });

            renderer->SubmitParticles(particleComponent, transform, particleComponent.MeshObj->GetMaterials()[0]);
        }

        {
            auto view = mRegistry.view<BoxColliderComponent>();
            for (auto entity : view)
            {
                Entity e = { entity, this };
                glm::mat4 transform = GetTransformRelativeToParent(e);
                auto& collider = e.GetComponent<BoxColliderComponent>();

                if (mSelectedEntity == entity)
                {
                    renderer->SubmitColliderMesh(collider, transform);
                }
            }
        }

        {
            auto view = mRegistry.view<SphereColliderComponent>();
            for (auto entity : view)
            {
                Entity e = { entity, this };
                glm::mat4 transform = GetTransformRelativeToParent(e);
                auto& collider = e.GetComponent<SphereColliderComponent>();

                if (mSelectedEntity == entity)
                {
                    renderer->SubmitColliderMesh(collider, transform);
                }
            }
        }

        {
            auto view = mRegistry.view<CapsuleColliderComponent>();
            for (auto entity : view)
            {
                Entity e = { entity, this };
                glm::mat4 transform = GetTransformRelativeToParent(e);
                auto& collider = e.GetComponent<CapsuleColliderComponent>();

                if (mSelectedEntity == entity)
                {
                    renderer->SubmitColliderMesh(collider, transform);
                }
            }
        }

        {
            auto view = mRegistry.view<MeshColliderComponent>();
            for (auto entity : view)
            {
                Entity e = { entity, this };
                glm::mat4 transform = GetTransformRelativeToParent(e);
                auto& collider = e.GetComponent<MeshColliderComponent>();

                if (mSelectedEntity == entity)
                {
                    renderer->SubmitColliderMesh(collider, transform);
                }
            }
        }

        renderer->EndScene();

        {
            const auto& camPosition = editorCamera.GetPosition();

            auto camDirection = glm::rotate(editorCamera.GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
            Audio::AudioEngine::Get().UpdateListenerPosition(camPosition, camDirection);
        }
        {
            auto view = mRegistry.view<Audio::AudioComponent>();

            std::vector<Entity> deadEntities;
            std::vector<Audio::SoundSourceUpdateData> updateData;
            updateData.reserve(view.size());

            for (auto entity : view)
            {
                Entity e = { entity, this };
                auto& audioComponent = e.GetComponent<Audio::AudioComponent>();

                if (audioComponent.AutoDestroy && audioComponent.MarkedForDestroy)
                {
                    deadEntities.push_back(e);
                    continue;
                }

                const auto& worldSpaceTransform = GetWorldSpaceTransform(e);

                glm::vec3 velocity{ 0.0f, 0.0f, 0.0f };
                updateData.emplace_back(Audio::SoundSourceUpdateData{
                    e.GetID(),
                    audioComponent.VolumeMultiplier,
                    audioComponent.PitchMultiplier,
                    worldSpaceTransform.Translation,
                    velocity });
            }

            Audio::AudioEngine::Get().SubmitSourceUpdateData(updateData);
            for (int i = deadEntities.size() - 1; i >= 0; --i)
            {
                DestroyEntity(deadEntities[i]);
            }
        }
    }

    void Scene::RenderSimulation(float dt, const EditorCamera& editorCamera)
    {
        {
            mLightEnvironment = LightEnvironment();
            auto lights = mRegistry.group<DirectionalLightComponent>(entt::get<TransformComponent>);
            uint32_t directionalLightIndex = 0;

            for (auto entity : lights)
            {
                auto [transformComponent, lightComponent] = lights.get<TransformComponent, DirectionalLightComponent>(entity);
                glm::vec3 direction = -glm::normalize(glm::mat3(transformComponent.GetTransform()) * glm::vec3(1.0f));
                mLightEnvironment.DirectionalLights[directionalLightIndex++] =
                {
                    direction,
                    lightComponent.Radiance,
                    lightComponent.Intensity,
                    lightComponent.CastShadows
                };
            }
        }
        {
            auto lights = mRegistry.group<SkyLightComponent>(entt::get<TransformComponent>);
            if (lights.empty())
            {
                mEnvironment = Ref<Environment>::Create(Renderer::GetBlackCubeTexture(), Renderer::GetBlackCubeTexture());
            }

            for (auto entity : lights)
            {
                auto [transformComponent, skyLightComponent] = lights.get<TransformComponent, SkyLightComponent>(entity);
                if (!skyLightComponent.SceneEnvironment && skyLightComponent.DynamicSky)
                {
                    Ref<TextureCube> preethamEnv = Renderer::CreatePreethamSky(skyLightComponent.TurbidityAzimuthInclination.x, skyLightComponent.TurbidityAzimuthInclination.y, skyLightComponent.TurbidityAzimuthInclination.z);
                    skyLightComponent.SceneEnvironment = Ref<Environment>::Create(preethamEnv, preethamEnv);
                }
                mEnvironment = skyLightComponent.SceneEnvironment;
                mEnvironmentIntensity = skyLightComponent.Intensity;
                if (mEnvironment)
                {
                    SetSkybox(mEnvironment->RadianceMap);
                }
            }
        }

        mSkyboxMaterial->Set("uUniforms.TextureLod", mSkyboxLod);
        auto group = mRegistry.group<MeshComponent>(entt::get<TransformComponent>);
        SceneRenderer::BeginScene(this, { editorCamera, editorCamera.GetViewMatrix(), 0.1f, 1000.0f, 45.0f });
        for (auto entity : group)
        {
            auto [meshComponent, transformComponent] = group.get<MeshComponent, TransformComponent>(entity);
            if (meshComponent.MeshObj && meshComponent.MeshObj->Type == AssetType::Mesh)
            {
                meshComponent.MeshObj->Update(dt);
                Entity e = Entity{ entity, this };
                glm::mat4 transform = GetTransformRelativeToParent(e);

                if (e.HasComponent<RigidBodyComponent>())
                {
                    transform = e.Transform().GetTransform();
                }

                SceneRenderer::SubmitMesh(meshComponent, transform);
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
        Audio::AudioEngine::SetSceneContext(this);

        {
            auto view = mRegistry.view<ScriptComponent>();
            for (auto entity : view)
            {
                Entity e = { entity, this };
                if (ScriptEngine::ModuleExists(e.GetComponent<ScriptComponent>().ModuleName))
                {
                    ScriptEngine::InstantiateEntityClass(e);
                }
            }
        }

        // Box2D physics
        auto sceneView = mRegistry.view<Box2DWorldComponent>();
        auto& world = mRegistry.get<Box2DWorldComponent>(sceneView.front()).World;
        {
            auto view = mRegistry.view<RigidBody2DComponent>();
            mPhysics2DBodyEntityBuffer = new Entity[view.size()];
            uint32_t physicsBodyEntityBufferIndex = 0;
            for (auto entity : view)
            {
                Entity e = { entity, this };
                UUID entityID = e.GetComponent<IDComponent>().ID;
                TransformComponent& transform = e.GetComponent<TransformComponent>();
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
                bodyDef.position.Set(transform.Translation.x, transform.Translation.y);

                bodyDef.angle = glm::radians(transform.Rotation.z);

                b2Body* body = world->CreateBody(&bodyDef);
                body->SetFixedRotation(rigidBody2D.FixedRotation);
                Entity* entityStorage = &mPhysics2DBodyEntityBuffer[physicsBodyEntityBufferIndex++];
                *entityStorage = e;
                body->GetUserData().pointer = (uintptr_t)entityStorage;
                rigidBody2D.RuntimeBody = body;
            }
        }

        {
            auto view = mRegistry.view<BoxCollider2DComponent>();
            for (auto entity : view)
            {
                Entity e = { entity, this };

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
                    fixtureDef.density = boxCollider2D.Density;
                    fixtureDef.friction = boxCollider2D.Friction;
                    body->CreateFixture(&fixtureDef);
                }
            }
        }

        {
            auto view = mRegistry.view<CircleCollider2DComponent>();
            for (auto entity : view)
            {
                Entity e = { entity, this };

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
                    fixtureDef.density = circleCollider2D.Density;
                    fixtureDef.friction = circleCollider2D.Friction;
                    body->CreateFixture(&fixtureDef);
                }
            }
        }

        PhysicsManager::CreateScene();

        Ref<Scene> thisScene = this;
        PhysicsManager::CreateActors(thisScene);

        {
            Entity mainCam = GetMainCameraEntity();
            Entity listener;

            auto view = mRegistry.view<AudioListenerComponent>();
            bool listenerFound = !view.empty();

            if (mainCam.mEntityHandle != entt::null)
            {
                if (!mainCam.HasComponent<AudioListenerComponent>())
                {
                    mainCam.AddComponent<AudioListenerComponent>();
                }
            }

            if (listenerFound)
            {
                for (auto entity : view)
                {
                    listener = { entity, this };
                    if (listener.GetComponent<AudioListenerComponent>().Active)
                    {
                        listenerFound = true;
                        break;
                    }
                    listenerFound = false;
                }
            }

            if (!listenerFound)
            {
                listener = mainCam;
            }

            if (listener.mEntityHandle != entt::null)
            {
                const auto& worldSpaceTransform = GetWorldSpaceTransform(listener);
                Audio::AudioEngine::Get().UpdateListenerPosition(worldSpaceTransform.Translation, worldSpaceTransform.Forward);
            }
        }
        {
            auto view = mRegistry.view<Audio::AudioComponent>();
            std::vector<Audio::SoundSourceUpdateData> updateData;
            updateData.reserve(view.size());

            for (auto entity : view)
            {
                const auto& [audioComponent] = view.get(entity);

                Entity e = { entity, this };
                const auto& worldSpaceTransform = GetWorldSpaceTransform(e);
                audioComponent.SourcePosition = worldSpaceTransform.Translation;

                glm::vec3 velocity{ 0.0f, 0.0f, 0.0f };
                updateData.emplace_back(Audio::SoundSourceUpdateData{
                    e.GetID(),
                    audioComponent.VolumeMultiplier,
                    audioComponent.PitchMultiplier,
                    worldSpaceTransform.Translation,
                    velocity });
            }

            Audio::AudioEngine::Get().SubmitSourceUpdateData(updateData);
        }

        mIsPlaying = true;
        mShouldSimulate = true;

        Audio::AudioEngine::RuntimePlaying(mSceneID);
    }

    void Scene::RuntimeStop()
    {
        Input::SetCursorMode(CursorMode::Normal);

        delete[] mPhysics2DBodyEntityBuffer;
        PhysicsManager::DestroyScene();
        mIsPlaying = false;
        mShouldSimulate = false;
    }

    void Scene::SimulationStart()
    {
        // Box2D physics
        auto sceneView = mRegistry.view<Box2DWorldComponent>();
        auto& world = mRegistry.get<Box2DWorldComponent>(sceneView.front()).World;

        {
            auto view = mRegistry.view<RigidBody2DComponent>();
            mPhysics2DBodyEntityBuffer = new Entity[view.size()];
            uint32_t physicsBodyEntityBufferIndex = 0;

            for (auto entity : view)
            {
                Entity e = { entity, this };
                UUID entityID = e.GetComponent<IDComponent>().ID;
                TransformComponent& transform = e.GetComponent<TransformComponent>();
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

                bodyDef.position.Set(transform.Translation.x, transform.Translation.y);
                bodyDef.angle = transform.Rotation.z;

                b2Body* body = world->CreateBody(&bodyDef);
                body->SetFixedRotation(rigidBody2D.FixedRotation);

                Entity* entityStorage = &mPhysics2DBodyEntityBuffer[physicsBodyEntityBufferIndex++];
                *entityStorage = e;
                body->GetUserData().pointer = reinterpret_cast<uintptr_t>(entityStorage);
                rigidBody2D.RuntimeBody = body;
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
                    fixtureDef.density = boxCollider2D.Density;
                    fixtureDef.friction = boxCollider2D.Friction;
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
                    fixtureDef.density = circleCollider2D.Density;
                    fixtureDef.friction = circleCollider2D.Friction;
                    body->CreateFixture(&fixtureDef);
                }
            }
        }

        PhysicsManager::CreateScene();

        Ref<Scene> thisScene = this;
        PhysicsManager::CreateActors(thisScene);

        mShouldSimulate = true;
    }

    void Scene::SimulationEnd()
    {
        Input::SetCursorMode(CursorMode::Normal);
        delete[] mPhysics2DBodyEntityBuffer;
        PhysicsManager::DestroyScene();
        mShouldSimulate = false;
    }

    void Scene::SetViewportSize(uint32_t width, uint32_t height)
    {
        mViewportWidth = width;
        mViewportHeight = height;
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
        //mSkyboxTexture = skybox;
        //mSkyboxMaterial->Set("uTexture", skybox);
    }

    Entity Scene::CreateEntity(const std::string& name)
    {
        auto entity = Entity{ mRegistry.create(), this };
        auto& idComponent = entity.AddComponent<IDComponent>();
        idComponent.ID = {};

        entity.AddComponent<TransformComponent>();
        entity.AddComponent<TagComponent>(name);

        entity.AddComponent<RelationshipComponent>();

        mEntityIDMap[idComponent.ID] = entity;
        return entity;
    }

    Entity Scene::CreateEntityWithID(UUID uuid, const std::string& name, bool runtimeMap)
    {
        auto entity = Entity{ mRegistry.create(), this };
        auto& idComponent = entity.AddComponent<IDComponent>();
        idComponent.ID = uuid;

        entity.AddComponent<TransformComponent>();
        entity.AddComponent<TagComponent>(name);

        entity.AddComponent<RelationshipComponent>();

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

        if (entity.HasComponent<Audio::AudioComponent>())
        {
            Audio::AudioEngine::Get().UnregisterAudioComponent(mSceneID, entity.GetID());
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
        CopyComponentIfExists<RelationshipComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<MeshComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<ParticleComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<DirectionalLightComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<SkyLightComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<PointLightComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<ScriptComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<CameraComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<SpriteRendererComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<RigidBody2DComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<BoxCollider2DComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<CircleCollider2DComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<RigidBodyComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<BoxColliderComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<SphereColliderComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<CapsuleColliderComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<MeshColliderComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<Audio::AudioComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<AudioListenerComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
    }

    Entity Scene::FindEntityByTag(const std::string& tag)
    {
        auto view = mRegistry.view<TagComponent>();
        for (auto entity : view)
        {
            const auto& canditate = view.get<TagComponent>(entity).Tag;
            if (canditate == tag)
            {
                return Entity(entity, this);
            }
        }

        return Entity{};
    }

    void Scene::ConvertToLocalSpace(Entity entity)
    {
        Entity parent = FindEntityByID(entity.GetParentID());
        if (!parent)
        {
            return;
        }

        auto& transform = entity.Transform();
        glm::mat4 parentTransform = GetWorldSpaceTransformMatrix(parent);

        glm::mat4 localTransform = glm::inverse(parentTransform) * transform.GetTransform();
        Math::DecomposeTransform(localTransform, transform.Translation, transform.Rotation, transform.Scale);
    }

    void Scene::ConvertToWorldSpace(Entity entity)
    {
        Entity parent = FindEntityByID(entity.GetParentID());
        if (!parent)
        {
            return;
        }

        glm::mat4 transform = GetTransformRelativeToParent(entity);

        auto& entityTransform = entity.Transform();
        Math::DecomposeTransform(transform, entityTransform.Translation, entityTransform.Rotation, entityTransform.Scale);
    }

    Entity Scene::FindEntityByID(UUID id)
    {
        auto view = mRegistry.view<IDComponent>();
        for (auto entity : view)
        {
            auto& idComponent = mRegistry.get<IDComponent>(entity);
            if (idComponent.ID == id)
            {
                return Entity(entity, this);
            }
        }

        return Entity{};
    }

    glm::mat4 Scene::GetTransformRelativeToParent(Entity entity)
    {
        glm::mat4 transform(1.0f);

        Entity parent = FindEntityByID(entity.GetParentID());
        if (parent)
        {
            transform = GetTransformRelativeToParent(parent);
        }

        return transform * entity.Transform().GetTransform();
    }

    glm::mat4 Scene::GetWorldSpaceTransformMatrix(Entity entity)
    {
        glm::mat4 transform = entity.Transform().GetTransform();
        while (Entity parent = FindEntityByID(entity.GetParentID()))
        {
            transform = parent.Transform().GetTransform() * transform;
            entity = parent;
        }
        return transform;
    }

    TransformComponent Scene::GetWorldSpaceTransform(Entity entity)
    {
        glm::mat4 transform = GetWorldSpaceTransformMatrix(entity);

        TransformComponent transformComponent;
        Math::DecomposeTransform(transform, transformComponent.Translation, transformComponent.Rotation, transformComponent.Scale);

        glm::quat rotationQuat = glm::quat(transformComponent.Rotation);
        transformComponent.Up = glm::normalize(glm::rotate(rotationQuat, glm::vec3(0.0f, 1.0f, 0.0f)));
        transformComponent.Right = glm::normalize(glm::rotate(rotationQuat, glm::vec3(1.0f, 0.0f, 0.0f)));
        transformComponent.Forward = glm::normalize(glm::rotate(rotationQuat, glm::vec3(0.0f, 0.0f, -1.0f)));

        return transformComponent;
    }

    //glm::mat4 Scene::GetWorldSpaceTransform(Entity entity)
    //{
    //    glm::mat4 transform = entity.Transform().GetTransform();
    //    while (Entity parent = FindEntityByUUID(entity.GetParentUUID()))
    //    {
    //        transform = parent.Transform().GetTransform() * transform;
    //        entity = parent;
    //    }
    //    return transform;
    //}

    void Scene::ParentEntity(Entity entity, Entity parent)
    {
        if (parent.IsDescendantOf(entity))
        {
            UnparentEntity(parent);

            Entity newParent = FindEntityByID(entity.GetParentID());
            if (newParent)
            {
                UnparentEntity(entity);
                ParentEntity(parent, newParent);
            }
        }
        else
        {
            Entity previousParent = FindEntityByID(entity.GetParentID());
            if (previousParent)
            {
                UnparentEntity(entity);
            }
        }

        entity.SetParentID(parent.GetID());
        parent.Children().push_back(entity.GetID());
        ConvertToLocalSpace(entity);
    }

    void Scene::UnparentEntity(Entity entity)
    {
        Entity parent = FindEntityByID(entity.GetParentID());
        if (!parent)
        {
            return;
        }

        auto& parentChildren = parent.Children();
        parentChildren.erase(std::remove(parentChildren.begin(), parentChildren.end(), entity.GetID()), parentChildren.end());
        ConvertToWorldSpace(entity);
        entity.SetParentID(0);
    }

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
        CopyComponent<RelationshipComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<MeshComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<ParticleComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<DirectionalLightComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<SkyLightComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<PointLightComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<ScriptComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<CameraComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<SpriteRendererComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<RigidBody2DComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<BoxCollider2DComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<CircleCollider2DComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<RigidBodyComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<BoxColliderComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<SphereColliderComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<CapsuleColliderComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<MeshColliderComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<Audio::AudioComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<AudioListenerComponent>(target->mRegistry, mRegistry, enttMap);

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
}