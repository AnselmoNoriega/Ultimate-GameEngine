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
    }

    void Scene::Update(float dt)
    {
        Camera* mainCamera = nullptr;
        glm::mat4* cameraTransform = nullptr;
        {
            auto group = mRegistry.view<TransformComponent, CameraComponent>();
            for (auto entity : group)
            {
                auto&& [tranform, camera] = group.get<TransformComponent, CameraComponent>(entity);

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
            //delete mainCamera;
            //mainCamera = nullptr;
            //delete cameraTransform;
            //cameraTransform = nullptr;
        }
    }

    Entity Scene::CreateEntity(const std::string& tagName)
    {
        Entity entity = { mRegistry.create(), this };
        entity.AddComponent<TransformComponent>();
        entity.AddComponent<TagComponent>(tagName);
        return entity;
    }
}