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

    void Scene::Update(float dt)
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
        glm::mat4* cameraTransform = nullptr;
        {
            auto view = mRegistry.view<TransformComponent, CameraComponent>();
            for (auto entity : view)
            {
                auto&& [tranform, camera] = view.get<TransformComponent, CameraComponent>(entity);

                if (camera.IsPrimary)
                {
                    mainCamera = &camera.Camera;
                    cameraTransform = &tranform.Transform;
                    break;
                }
            }
        }

        if (mainCamera)
        {
            Renderer2D::BeginScene(*mainCamera, *cameraTransform);

            auto group = mRegistry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
            for (auto entity : group)
            {
                auto&& [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

                Renderer2D::DrawQuad(transform, sprite.Color);
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

    void Scene::ViewportResize(uint32_t width, uint32_t height)
    {
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
}