#pragma once

#include "entt.hpp"

namespace NR
{
    class Entity;

    class Scene
    {
    public:
        Scene();
        ~Scene();

        void Update(float dt);

        Entity CreateEntity(const std::string& tagName = "Object");

    private:
        entt::registry mRegistry;

        friend class Entity;
    };
}