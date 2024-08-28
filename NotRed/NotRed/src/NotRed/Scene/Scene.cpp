#include "nrpch.h"
#include "Scene.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <box2d/box2d.h>

#include "NotRed/Physics/PhysicsManager.h"

#include "NotRed/Core/Input.h"

#include "Entity.h"
#include "Components.h"

#include "NotRed/Renderer/Renderer2D.h"
#include "NotRed/Renderer/SceneRenderer.h"
#include "NotRed/Script/ScriptEngine.h"

namespace NR
{
    std::unordered_map<UUID, Scene*> sActiveScenes;

    static physx::PxDefaultErrorCallback sPXErrorCallback;
    static physx::PxDefaultAllocator sPXAllocator;
    static physx::PxFoundation* sPXFoundation;

    struct SceneComponent
    {
        UUID SceneID;
    };

    struct PhysicsSceneComponent
    {
        physx::PxScene* World;
    };

    struct Box2DWorldComponent
    {
        std::unique_ptr<b2World> World;
    };

    class ContactListener2D : public b2ContactListener
    {
    public:
        virtual void BeginContact(b2Contact* contact) override
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
        virtual void EndContact(b2Contact* contact) override
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

        virtual void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override
        {
            B2_NOT_USED(contact);
            B2_NOT_USED(oldManifold);
        }

        virtual void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) override
        {
            B2_NOT_USED(contact);
            B2_NOT_USED(impulse);
        }
    };

    class ContactListener3D : public physx::PxSimulationEventCallback
    {
    public:
        virtual void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) override
        {
            PX_UNUSED(constraints);
            PX_UNUSED(count);
        }

        virtual void onWake(physx::PxActor** actors, physx::PxU32 count) override
        {
            PX_UNUSED(actors);
            PX_UNUSED(count);
        }

        virtual void onSleep(physx::PxActor** actors, physx::PxU32 count) override
        {
            PX_UNUSED(actors);
            PX_UNUSED(count);
        }

        virtual void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) override
        {
            Entity& a = *(Entity*)pairHeader.actors[0]->userData;
            Entity& b = *(Entity*)pairHeader.actors[1]->userData;

            if (pairs->flags == physx::PxContactPairFlag::eACTOR_PAIR_HAS_FIRST_TOUCH)
            {
                if (a.HasComponent<ScriptComponent>() && ScriptEngine::ModuleExists(a.GetComponent<ScriptComponent>().ModuleName))
                {
                    ScriptEngine::CollisionBegin(a);
                }

                if (b.HasComponent<ScriptComponent>() && ScriptEngine::ModuleExists(b.GetComponent<ScriptComponent>().ModuleName))
                {
                    ScriptEngine::CollisionBegin(b);
                }
            }
            else if (pairs->flags == physx::PxContactPairFlag::eACTOR_PAIR_LOST_TOUCH)
            {
                if (a.HasComponent<ScriptComponent>() && ScriptEngine::ModuleExists(a.GetComponent<ScriptComponent>().ModuleName))
                {
                    ScriptEngine::CollisionEnd(a);
                }

                if (b.HasComponent<ScriptComponent>() && ScriptEngine::ModuleExists(b.GetComponent<ScriptComponent>().ModuleName))
                {
                    ScriptEngine::CollisionEnd(b);
                }
            }
        }

        virtual void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override
        {
            PX_UNUSED(pairs);
            PX_UNUSED(count);
        }

        virtual void onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) override
        {
            PX_UNUSED(bodyBuffer);
            PX_UNUSED(poseBuffer);
            PX_UNUSED(count);
        }
    };

    static ContactListener2D sBox2DContactListener;
    static ContactListener3D sPhysicsContactListener;

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

        auto entityID = registry.get<IDComponent>(entity).ID;
        ScriptEngine::ScriptComponentDestroyed(sceneID, entityID);
    }

    Scene::Scene(const std::string& debugName)
        : mDebugName(debugName)
    {
        mRegistry.on_construct<ScriptComponent>().connect<&ScriptComponentConstruct>();
        mRegistry.on_destroy<ScriptComponent>().connect<&ScriptComponentDestroy>();

        mSceneEntity = mRegistry.create();
        mRegistry.emplace<SceneComponent>(mSceneEntity, mSceneID);

        Box2DWorldComponent& b2dWorld = mRegistry.emplace<Box2DWorldComponent>(mSceneEntity, std::make_unique<b2World>(b2Vec2{ 0.0f, -9.8f }));
        b2dWorld.World->SetContactListener(&sBox2DContactListener);

        sActiveScenes[mSceneID] = this;

        physx::PxSceneDesc sceneDesc = PhysicsManager::CreateSceneDesc();
        sceneDesc.gravity = physx::PxVec3(0.0f, -9.8F, 0.0f);
        sceneDesc.simulationEventCallback = &sPhysicsContactListener;

        PhysicsSceneComponent& physxWorld = mRegistry.emplace<PhysicsSceneComponent>(mSceneEntity, PhysicsManager::CreateScene(sceneDesc));
        NR_CORE_ASSERT(physxWorld.World);

        Init();
        SetEnvironment(Environment::Load("Assets/Env/HDR_blue_nebulae-1.hdr"));
    }

    Scene::~Scene()
    {
        mRegistry.on_destroy<ScriptComponent>().disconnect();

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

    void Scene::Shutdown()
    {
        auto physxView = mRegistry.view<PhysicsSceneComponent>();
        mRegistry.get<PhysicsSceneComponent>(mSceneEntity).World->release();
    }

    static std::tuple<glm::vec3, glm::quat, glm::vec3> DecomposeTransform(const glm::mat4& transform)
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
                auto [translation, rotationQuat, scale] = DecomposeTransform(transform);
                glm::vec3 rotation = glm::eulerAngles(rotationQuat);

                transform = glm::translate(glm::mat4(1.0f), { position.x, position.y, transform[3].z }) *
                    glm::toMat4(glm::quat({ rotation.x, rotation.y, body->GetAngle() })) *
                    glm::scale(glm::mat4(1.0f), scale);
            }
        }

        auto physicsView = mRegistry.view<PhysicsSceneComponent>();
        physx::PxScene* physicsScene = mRegistry.get<PhysicsSceneComponent>(physicsView.front()).World;
        constexpr float stepSize = 0.016666660f;
        physicsScene->simulate(stepSize);
        physicsScene->fetchResults(true);

        {
            auto view = mRegistry.view<RigidBodyComponent>();
            for (auto entity : view)
            {
                Entity e = { entity, this };

                auto& transform = e.Transform();
                RigidBodyComponent& rb = e.GetComponent<RigidBodyComponent>();
                physx::PxRigidActor* actor = static_cast<physx::PxRigidActor*>(rb.RuntimeActor);

                if (rb.BodyType == RigidBodyComponent::Type::Dynamic)
                {
                    const physx::PxVec3& position = actor->getGlobalPose().p;
                    const physx::PxQuat& physicsBodyRotation = actor->getGlobalPose().q;

                    auto [translation, rotationQuat, scale] = DecomposeTransform(transform);
                    glm::vec3 rotation = glm::eulerAngles(rotationQuat);

                    float xAngle, yAngle, zAngle;
                    physx::PxVec3 axis;
                    physicsBodyRotation.toRadiansAndUnitAxis(xAngle, axis);
                    physicsBodyRotation.toRadiansAndUnitAxis(yAngle, axis);
                    physicsBodyRotation.toRadiansAndUnitAxis(zAngle, axis);

                    transform = glm::translate(glm::mat4(1.0f), { position.x, position.y, position.z }) *
                        glm::toMat4(glm::quat({ xAngle, yAngle, zAngle })) *
                        glm::scale(glm::mat4(1.0f), scale);
                }
                else if (rb.BodyType == RigidBodyComponent::Type::Static)
                {
                    actor->setGlobalPose(PhysicsManager::CreatePose(transform));
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
            auto&& [meshComponent, transformComponent] = group.get<MeshComponent, TransformComponent>(entity);
            if (meshComponent.MeshObj)
            {
                meshComponent.MeshObj->Update(dt);

                if (mSelectedEntity == entity)
                {
                    SceneRenderer::SubmitSelectedMesh(meshComponent, transformComponent);
                }
                else
                {
                    SceneRenderer::SubmitMesh(meshComponent, transformComponent);
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
            mPhysics2DBodyEntityBuffer = new Entity[view.size()];
            uint32_t physicsBodyEntityBufferIndex = 0;
            for (auto entity : view)
            {
                Entity e = { entity, this };
                UUID entityID = e.GetComponent<IDComponent>().ID;
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

                auto [translation, rotationQuat, scale] = DecomposeTransform(transform);
                glm::vec3 rotation = glm::eulerAngles(rotationQuat);
                bodyDef.angle = rotation.z;

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


        {
            auto physxView = mRegistry.view<PhysicsSceneComponent>();
            physx::PxScene* physxWorld = mRegistry.get<PhysicsSceneComponent>(physxView.front()).World;

            auto view = mRegistry.view<RigidBodyComponent>();
            mPhysics3DBodyEntityBuffer = new Entity[view.size()];
            uint32_t physicsBodyEntityBufferIndex = 0;

            for (auto entity : view)
            {
                Entity e = { entity, this };
                UUID entityID = e.GetComponent<IDComponent>().ID;
                auto& transform = e.Transform();
                auto& rigidbody = mRegistry.get<RigidBodyComponent>(entity);

                physx::PxRigidActor* actor = PhysicsManager::AddActor(physxWorld, rigidbody, transform);
                NR_CORE_ASSERT(actor);

                Entity* entityStorage = &mPhysics3DBodyEntityBuffer[physicsBodyEntityBufferIndex++];
                *entityStorage = e;
                actor->userData = (void*)entityStorage;

                rigidbody.RuntimeActor = actor;
            }
        }

        {
            auto view = mRegistry.view<BoxColliderComponent>();
            for (auto entity : view)
            {
                Entity e = { entity, this };
                auto& transform = e.Transform();

                auto& boxCollider = mRegistry.get<BoxColliderComponent>(entity);
                if (e.HasComponent<RigidBodyComponent>())
                {
                    auto& rigidBody = e.GetComponent<RigidBodyComponent>();
                    auto& physicsMaterial = e.GetComponent<PhysicsMaterialComponent>();
                    NR_CORE_ASSERT(rigidBody.RuntimeActor);
                    physx::PxRigidActor* actor = static_cast<physx::PxRigidActor*>(rigidBody.RuntimeActor);

                    physx::PxBoxGeometry boxGeometry = physx::PxBoxGeometry(boxCollider.Size.x / 2.0f, boxCollider.Size.y / 2.0f, boxCollider.Size.z / 2.0f);
                    physx::PxMaterial* material = PhysicsManager::CreateMaterial(physicsMaterial.StaticFriction, physicsMaterial.DynamicFriction, physicsMaterial.Bounciness);
                    physx::PxShape* shape = physx::PxRigidActorExt::createExclusiveShape(*actor, boxGeometry, *material);
                    shape->setLocalPose(PhysicsManager::CreatePose(glm::translate(glm::mat4(1.0f), boxCollider.Offset)));
                }
            }
        }

        {
            auto view = mRegistry.view<SphereColliderComponent>();
            for (auto entity : view)
            {
                Entity e = { entity, this };
                auto& transform = e.Transform();

                auto& sphereCollider = mRegistry.get<SphereColliderComponent>(entity);
                if (e.HasComponent<RigidBodyComponent>())
                {
                    auto& rigidBody = e.GetComponent<RigidBodyComponent>();
                    auto& physicsMaterial = e.GetComponent<PhysicsMaterialComponent>();
                    NR_CORE_ASSERT(rigidBody.RuntimeActor);
                    physx::PxRigidActor* actor = static_cast<physx::PxRigidActor*>(rigidBody.RuntimeActor);

                    physx::PxSphereGeometry sphereGeometry = physx::PxSphereGeometry(sphereCollider.Radius);
                    physx::PxMaterial* material = PhysicsManager::CreateMaterial(physicsMaterial.StaticFriction, physicsMaterial.DynamicFriction, physicsMaterial.Bounciness);
                    physx::PxShape* shape = physx::PxRigidActorExt::createExclusiveShape(*actor, sphereGeometry, *material);
                    shape->setLocalPose(PhysicsManager::CreatePose(glm::translate(glm::mat4(1.0f), sphereCollider.Offset)));

                    physx::PxRigidDynamic* rigidBodyActor = actor->is<physx::PxRigidDynamic>();

                    if (rigidBodyActor)
                    {
                        rigidBodyActor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, true);
                        rigidBodyActor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, true);
                        rigidBodyActor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, true);
                    }
                }
            }
        }

        {
            auto view = mRegistry.view<CapsuleColliderComponent>();
            for (auto entity : view)
            {
                Entity e = { entity, this };
                auto& transform = e.Transform();

                auto& capsuleCollider = mRegistry.get<CapsuleColliderComponent>(entity);
                if (e.HasComponent<RigidBodyComponent>())
                {
                    auto& rigidBody = e.GetComponent<RigidBodyComponent>();
                    auto& physicsMaterial = e.GetComponent<PhysicsMaterialComponent>();
                    NR_CORE_ASSERT(rigidBody.RuntimeActor);
                    physx::PxRigidActor* actor = static_cast<physx::PxRigidActor*>(rigidBody.RuntimeActor);

                    physx::PxCapsuleGeometry capsuleGeometry = physx::PxCapsuleGeometry(capsuleCollider.Radius, capsuleCollider.Height / 2.0f);
                    physx::PxMaterial* material = PhysicsManager::CreateMaterial(physicsMaterial.StaticFriction, physicsMaterial.DynamicFriction, physicsMaterial.Bounciness);
                    physx::PxShape* shape = physx::PxRigidActorExt::createExclusiveShape(*actor, capsuleGeometry, *material);
                    //shape->setLocalPose(PhysicsManager::CreatePose(glm::translate(glm::mat4(1.0f), capsuleCollider.Offset)));

                    shape->setLocalPose(physx::PxTransform(physx::PxQuat(physx::PxHalfPi, physx::PxVec3(0, 0, 1))));
                }
            }
        }

        {
            auto view = mRegistry.view<MeshColliderComponent>();
            for (auto entity : view)
            {
                Entity e = { entity, this };
                auto& transform = e.Transform();

                auto& meshCollider = mRegistry.get<MeshColliderComponent>(entity);
                if (e.HasComponent<RigidBodyComponent>())
                {
                    auto& rigidBody = e.GetComponent<RigidBodyComponent>();
                    auto& physicsMaterial = e.GetComponent<PhysicsMaterialComponent>();
                    NR_CORE_ASSERT(rigidBody.RuntimeActor);
                    physx::PxRigidActor* actor = static_cast<physx::PxRigidActor*>(rigidBody.RuntimeActor);

                    physx::PxConvexMesh* triangleMesh = PhysicsManager::CreateMeshCollider(meshCollider);
                    NR_CORE_ASSERT(triangleMesh);

                    physx::PxConvexMeshGeometry triangleGeometry = physx::PxConvexMeshGeometry(triangleMesh);
                    physx::PxMaterial* material = PhysicsManager::CreateMaterial(physicsMaterial.StaticFriction, physicsMaterial.DynamicFriction, physicsMaterial.Bounciness);
                    physx::PxRigidActorExt::createExclusiveShape(*actor, triangleGeometry, *material);
                }
            }
        }

        {
            auto view = mRegistry.view<RigidBodyComponent>();
            for (auto entity : view)
            {
                Entity e = { entity, this };
                auto& rigidBody = e.GetComponent<RigidBodyComponent>();
                NR_CORE_ASSERT(rigidBody.RuntimeActor);
                physx::PxRigidActor* actor = static_cast<physx::PxRigidActor*>(rigidBody.RuntimeActor);

                if (rigidBody.BodyType == RigidBodyComponent::Type::Static)
                {
                    PhysicsManager::SetCollisionFilters(actor, (uint32_t)FilterGroup::Static, (uint32_t)FilterGroup::All);
                }
                else if (rigidBody.BodyType == RigidBodyComponent::Type::Dynamic)
                {
                    PhysicsManager::SetCollisionFilters(actor, (uint32_t)FilterGroup::Dynamic, (uint32_t)FilterGroup::All);
                }
            }
        }

        mIsPlaying = true;
    }

    void Scene::RuntimeStop()
    {
        auto physxView = mRegistry.view<PhysicsSceneComponent>();
        mRegistry.get<PhysicsSceneComponent>(mSceneEntity).World->release();

        delete[] mPhysics3DBodyEntityBuffer;
        delete[] mPhysics2DBodyEntityBuffer;
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
        CopyComponentIfExists<RigidBodyComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<PhysicsMaterialComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<BoxColliderComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<SphereColliderComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
        CopyComponentIfExists<MeshColliderComponent>(newEntity.mEntityHandle, entity.mEntityHandle, mRegistry);
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
        CopyComponent<RigidBodyComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<PhysicsMaterialComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<BoxColliderComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<SphereColliderComponent>(target->mRegistry, mRegistry, enttMap);
        CopyComponent<MeshColliderComponent>(target->mRegistry, mRegistry, enttMap);

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