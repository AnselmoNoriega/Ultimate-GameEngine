#pragma once

#include "Scene.h"

#include "entt.hpp"

namespace NR
{
    class Entity
    {
    public:
        Entity() = default;
        Entity(entt::entity entity, Scene* scene);
        Entity(const Entity& other) = default;

        template<typename T, typename... Args>
        T& AddComponent(Args&&... args)
        {
            NR_CORE_ASSERT(!HasComponent<T>(), "This Entity already has this component!");
            return mScene->mRegistry.emplace<T>(mEntity, std::forward<Args>(args)...);
        }

        template<typename T>
        T& GetComponent()
        {
            NR_CORE_ASSERT(HasComponent<T>(), "This Entity does not have this component!");
            return mScene->mRegistry.get<T>(mEntity);
        }

        template<typename T>
        bool HasComponent()
        {
            return mScene->mRegistry.all_of<T>(mEntity);
        }

        template<typename T>
        bool RemoveComponent()
        {
            NR_CORE_ASSERT(HasComponent<T>(), "This Entity does not have this component!");
            return mScene->mRegistry.remove<T>(mEntity);
        }

        operator bool() const { return (uint32_t)mEntity != entt::null; }

    private:
        entt::entity mEntity{ entt::null };
        Scene* mScene = nullptr;
    };
}