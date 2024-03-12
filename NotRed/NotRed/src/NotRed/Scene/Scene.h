#pragma once

#include "entt.hpp"

namespace NR
{
    class Scene
    {
    public:
        Scene();
        ~Scene();

        void Update(float dt);

        entt::entity CreateEntity();

        entt::registry& Reg() { return mRegistry; }

    private:
        entt::registry mRegistry;
    };
}