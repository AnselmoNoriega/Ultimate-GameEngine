#include "nrpch.h"
#include "Scene.h"

#include "Components.h"
#include "ScriptableEntity.h"
#include "NotRed/Renderer/Renderer2D.h"
#include "NotRed/Scripting/ScriptEngine.h"

#include "box2d/b2_world.h"
#include "box2d/b2_body.h"
#include "box2d/b2_fixture.h"
#include "box2d/b2_polygon_shape.h"
#include "box2d/b2_circle_shape.h"

namespace NR
{
    Scene::Scene()
    {

    }

    Scene::~Scene()
    {
        mRegistry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
            {
                if (nsc.Instance)
                {
                    nsc.DestroyInstanceFunction();
                }
            });
    }

    template<typename... Component>
    static void CopyComponent(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
    {
        ([&]()
        {
            auto view = src.view<Component>();
            for (auto e : view)
            {
                UUID uuid = src.get<IDComponent>(e).ID;
                entt::entity enttIDdst = enttMap.at(uuid);

                auto& component = src.get<Component>(e);
                dst.emplace_or_replace<Component>(enttIDdst, component);
            }
        }(), ...);
    }

    template<typename... Component>
    static void CopyComponent(ComponentGroup<Component...>, entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
    {
        CopyComponent<Component...>(dst, src, enttMap);
    }

    template<typename... Component>
    static void CopyComponentIfExists(Entity dst, Entity src)
    {
        ([&]()
        {
            if (src.HasComponent<Component>())
            {
                dst.AddOrReplaceComponent<Component>(src.GetComponent<Component>());
            }
        }(), ...);
    }

    template<typename... Component>
    static void CopyComponentIfExists(ComponentGroup<Component...>, Entity dst, Entity src)
    {
        CopyComponentIfExists<Component...>(dst, src);
    }

    Ref<Scene> Scene::Copy(Ref<Scene> other)
    {
        Ref<Scene> newScene = CreateRef<Scene>();

        newScene->mViewportWidth = other->mViewportWidth;
        newScene->mViewportHeight = other->mViewportHeight;
        
        auto& srcSceneRegistry = other->mRegistry;
        auto& dstSceneRegistry = newScene->mRegistry;
        std::unordered_map<UUID, entt::entity> enttMap;

        auto idView = srcSceneRegistry.view<IDComponent>();
        for (auto e : idView)
        {
            UUID uuid = srcSceneRegistry.get<IDComponent>(e).ID;
            const auto& name = srcSceneRegistry.get<TagComponent>(e).Tag;
            enttMap[uuid] = (entt::entity)newScene->CreateEntityWithUUID(uuid, name);
        }

        CopyComponent(AllComponents{}, dstSceneRegistry, srcSceneRegistry, enttMap);

        return newScene;
    }

    void Scene::UpdateEditor(float dt, EditorCamera& camera)
    {
        Renderer2D::BeginScene(camera);

        {
            auto group = mRegistry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
            for (auto entity : group)
            {
                auto&& [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

                Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity);
            }
        }
        {
            auto view = mRegistry.view<TransformComponent, CircleRendererComponent>();
            for (auto entity : view)
            {
                auto&& [transform, circle] = view.get<TransformComponent, CircleRendererComponent>(entity);

                Renderer2D::DrawCircle(transform.GetTransform(), circle.Color, circle.Thickness, (int)entity);
            }
        }

        Renderer2D::EndScene();
    }

    void Scene::UpdatePlay(float dt)
    {
        Camera* mainCamera = nullptr;
        glm::mat4 cameraTransform;
        {
            auto view = mRegistry.view<TransformComponent, CameraComponent>();
            for (auto entity : view)
            {
                auto&& [tranform, camera] = view.get<TransformComponent, CameraComponent>(entity);

                if (camera.IsPrimary)
                {
                    mainCamera = &camera.Camera;
                    cameraTransform = tranform.GetTransform();
                    break;
                }
            }
        }

        if (mainCamera)
        {
            Renderer2D::BeginScene(*mainCamera, cameraTransform);

            {
                auto group = mRegistry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
                for (auto entity : group)
                {
                    auto&& [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

                    Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity);
                }
            }
            {
                auto view = mRegistry.view<TransformComponent, CircleRendererComponent>();
                for (auto entity : view)
                {
                    auto&& [transform, circle] = view.get<TransformComponent, CircleRendererComponent>(entity);

                    Renderer2D::DrawCircle(transform.GetTransform(), circle.Color, circle.Thickness, (int)entity);
                }
            }

            Renderer2D::EndScene();
        }
    }

    void Scene::UpdateRunTime(float dt)
    {
        if (mIsPaused)
        {
            return;
        }

        {
            auto view = mRegistry.view<ScriptComponent>();
            for (auto e : view)
            {
                Entity entity = { e, this };
                ScriptEngine::UpdateEntity(entity, dt);
            }

            mRegistry.view<NativeScriptComponent>().each([=](auto entity, NativeScriptComponent& nsc)
                {
                    if (!nsc.Instance)
                    {
                        nsc.InstantiateFunction();
                        nsc.Instance->mEntity = { entity, this };
                        nsc.CreateFunction();
                    }

                    nsc.UpdateFunction(dt);
                });
        }

        {
            mPhysicsWorld->Step(dt, mVelocityIterations, mPositionIterations);

            auto view = mRegistry.view<Rigidbody2DComponent>();

            for (auto e : view)
            {
                Entity entity = { e, this };
                auto& transform = entity.GetComponent<TransformComponent>();
                auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

                b2Body* body = (b2Body*)rb2d.RuntimeBody;
                const auto& position = body->GetPosition();
                transform.Translation.x = position.x;
                transform.Translation.y = position.y;
                transform.Rotation.z = body->GetAngle();
            }
        }
    }

    Entity Scene::CreateEntity(const std::string& tagName)
    {
        return CreateEntityWithUUID(UUID(), tagName);
    }

    Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& tagName)
    {
        Entity entity = { mRegistry.create(), this };
        entity.AddComponent<IDComponent>(uuid);
        entity.AddComponent<TransformComponent>();
        entity.AddComponent<TagComponent>(tagName);

        mEntities[uuid] = entity;
        return entity;
    }

    void Scene::RemoveEntity(Entity entity)
    {
        mEntities.erase(entity.GetUUID());
        mRegistry.destroy(entity);
    }

    Entity Scene::GetEntity(UUID uuid)
    {
        if (mEntities.find(uuid) != mEntities.end())
        {
            return { mEntities.at(uuid), this };
        }

        return {};
    }

    void Scene::DuplicateEntity(Entity entity)
    {
        std::string name = entity.GetName();
        Entity newEntity = CreateEntity(name);

        CopyComponentIfExists(AllComponents{}, newEntity, entity);
    }

    void Scene::RuntimeStart()
    {
        mIsRunning = true;

        mPhysicsWorld = new b2World({ 0.0f, -9.81f });

        auto view = mRegistry.view<Rigidbody2DComponent>();
        for (auto e : view)
        {
            Entity entity = { e, this };
            auto& transform = entity.GetComponent<TransformComponent>();
            auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

            b2BodyDef bodyDef;
            bodyDef.type = (b2BodyType)rb2d.Type;
            bodyDef.position.Set(transform.Translation.x, transform.Translation.y);
            bodyDef.angle = transform.Rotation.z;

            b2Body* body = mPhysicsWorld->CreateBody(&bodyDef);
            body->SetFixedRotation(rb2d.FixedRotation);
            rb2d.RuntimeBody = body;

            if (entity.HasComponent<BoxCollider2DComponent>())
            {
                auto& box2D = entity.GetComponent<BoxCollider2DComponent>();

                b2PolygonShape boxShape;
                boxShape.SetAsBox(box2D.Size.x * transform.Scale.x, box2D.Size.y * transform.Scale.y);

                b2FixtureDef fixtureDef;
                fixtureDef.shape = &boxShape;
                fixtureDef.density = rb2d.Density;
                fixtureDef.friction = rb2d.Friction;
                fixtureDef.restitution = rb2d.Restitution;
                fixtureDef.restitutionThreshold = rb2d.RestitutionThreshold;
                body->CreateFixture(&fixtureDef);
            }

            if (entity.HasComponent<CircleCollider2DComponent>())
            {
                auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();

                b2CircleShape circleShape;
                circleShape.m_p.Set(cc2d.Offset.x, cc2d.Offset.y);
                circleShape.m_radius = transform.Scale.x * cc2d.Radius;

                b2FixtureDef fixtureDef;
                fixtureDef.shape = &circleShape;
                fixtureDef.density = rb2d.Density;
                fixtureDef.friction = rb2d.Friction;
                fixtureDef.restitution = rb2d.Restitution;
                fixtureDef.restitutionThreshold = rb2d.RestitutionThreshold;
                body->CreateFixture(&fixtureDef);
            }
        }

        {
            ScriptEngine::RuntimeStart(this);
            // Instantiate all script entities

            auto view = mRegistry.view<ScriptComponent>();
            for (auto e : view)
            {
                Entity entity = { e, this };
                ScriptEngine::ConstructEntity(entity);
            }
        }
    }

    void Scene::RuntimeStop()
    {
        mIsRunning = false;

        delete mPhysicsWorld;
        mPhysicsWorld = nullptr;

        ScriptEngine::RuntimeStop();
    }

    void Scene::ViewportResize(uint32_t width, uint32_t height)
    {
        mViewportWidth = width;
        mViewportHeight = height;

        auto view = mRegistry.view<CameraComponent>();
        for (auto entity : view)
        {
            auto& camera = view.get<CameraComponent>(entity);
            if (!camera.HasFixedAspectRatio)
            {
                camera.Camera.SetViewportSize(width, height);
            }
        }
    }

    Entity Scene::GetPrimaryCameraEntity()
    {
        auto view = mRegistry.view<CameraComponent>();
        for (auto entity : view)
        {
            const auto& camera = view.get<CameraComponent>(entity);
            if (camera.IsPrimary)
            {
                return Entity{ entity, this };
            }
        }
        return {};
    }

    template<typename T>
    void Scene::ComponentAdded(Entity entity, T& component) {}

    template<>
    void Scene::ComponentAdded<IDComponent>(Entity entity, IDComponent& component) {}

    template<>
    void Scene::ComponentAdded<TagComponent>(Entity entity, TagComponent& component) {}

    template<>
    void Scene::ComponentAdded<TransformComponent>(Entity entity, TransformComponent& component) {}

    template<>
    void Scene::ComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent& component) {}

    template<>
    void Scene::ComponentAdded<CircleRendererComponent>(Entity entity, CircleRendererComponent& component) {}

    template<>
    void Scene::ComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
    {
        if (mViewportWidth > 0 && mViewportHeight > 0)
        {
            component.Camera.SetViewportSize(mViewportWidth, mViewportHeight);
        }
    }

    template<>
    void Scene::ComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component) {}

    template<>
    void Scene::ComponentAdded<Rigidbody2DComponent>(Entity entity, Rigidbody2DComponent& component) {}

    template<>
    void Scene::ComponentAdded<BoxCollider2DComponent>(Entity entity, BoxCollider2DComponent& component) {}

    template<>
    void Scene::ComponentAdded<CircleCollider2DComponent>(Entity entity, CircleCollider2DComponent& component) {}
}