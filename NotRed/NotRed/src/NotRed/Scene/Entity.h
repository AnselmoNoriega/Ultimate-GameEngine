#pragma once

#include "entt.hpp"

#include "Scene.h"
#include "NotRed/Core/UUID.h"
#include "Components.h"

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
            T& component = mScene->mRegistry.emplace<T>(mEntity, std::forward<Args>(args)...);
            mScene->ComponentAdded<T>(*this, component);
            return component;
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

        template<typename T, typename... Args>
        T& AddOrReplaceComponent(Args&&... args)
        {
            T& component = mScene->mRegistry.emplace_or_replace<T>(mEntity, std::forward<Args>(args)...);
            mScene->ComponentAdded<T>(*this, component);
            return component;
        }

        operator bool() const { return mEntity != entt::null; }
        operator entt::entity() const { return mEntity; }
        operator uint32_t() const { return (uint32_t)mEntity; }

        UUID GetUUID() { return GetComponent<IDComponent>().ID; }
        const std::string& GetName() { return GetComponent<TagComponent>().Tag; }

        bool operator==(const Entity& other) const
        {
            return mEntity == other.mEntity && mScene == other.mScene;
        }

        bool operator!=(const Entity& other) const
        {
            return !(*this == other);
        }

    private:
        entt::entity mEntity{ entt::null };
        Scene* mScene = nullptr;
    };
}