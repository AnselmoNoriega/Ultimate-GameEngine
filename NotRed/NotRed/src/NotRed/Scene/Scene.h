#pragma once

#include "entt.hpp"

namespace NR
{
    class Scene
    {
    public:
        Scene();
        ~Scene();

    private:
        entt::registry mRegistry;
    };
}