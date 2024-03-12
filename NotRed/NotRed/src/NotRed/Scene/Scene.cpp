#include "nrpch.h"
#include "Scene.h"

#include "Components.h"
#include "NotRed/Renderer/Renderer2D.h"

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
        auto group = mRegistry.group<TransformComponent>(entt::get<SpriteRendererComponent>);

        for (auto enttity : group)
        {
            auto&& [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(enttity);

            Renderer2D::DrawQuad(transform, sprite.Color);
        }
    }

    entt::entity Scene::CreateEntity()
    {
        return mRegistry.create();
    }
}