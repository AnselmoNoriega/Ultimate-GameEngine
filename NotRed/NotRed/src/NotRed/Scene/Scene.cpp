#include "nrpch.h"
#include "Scene.h"

#include "Components.h"
#include "NotRed/Renderer/Renderer2D.h"

#include "Entity.h"

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

    void Scene::UpdateEditor(float dt, EditorCamera& camera)
    {
        Renderer2D::BeginScene(camera);

        auto group = mRegistry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
        for (auto entity : group)
        {
            auto&& [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

            Renderer2D::DrawQuad(transform.GetTransform(), sprite.Color);
        }

        Renderer2D::EndScene();
    }

    void Scene::UpdateRunTime(float dt)
    {
        {
            mRegistry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
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

            auto group = mRegistry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
            for (auto entity : group)
            {
                auto&& [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

                Renderer2D::DrawQuad(transform.GetTransform(), sprite.Color);
            }

            Renderer2D::EndScene();
        }
    }

    Entity Scene::CreateEntity(const std::string& tagName)
    {
        Entity entity = { mRegistry.create(), this };
        entity.AddComponent<TransformComponent>();
        entity.AddComponent<TagComponent>(tagName);
        return entity;
    }

    void Scene::RemoveEntity(Entity entity)
    {
        mRegistry.destroy(entity);
    }

    void Scene::ViewportResize(uint32_t width, uint32_t height)
    {
        mWidth = width;
        mHeight = height;

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
    void Scene::ComponentAdded(Entity entity, T& component)
    {
    }

    template<>
    void Scene::ComponentAdded<TagComponent>(Entity entity, TagComponent& component)
    {

    }

    template<>
    void Scene::ComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
    {

    }

    template<>
    void Scene::ComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent& component)
    {

    }

    template<>
    void Scene::ComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
    {
        component.Camera.SetViewportSize(mWidth, mHeight);
    }

    template<>
    void Scene::ComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component)
    {

    }
}