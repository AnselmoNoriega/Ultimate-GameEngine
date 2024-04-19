#pragma once

#include "entt.hpp"

#include "NotRed/Core/UUID.h"
#include "NotRed/Renderer/EditorCamera.h"

class b2World;

namespace NR
{
    class Entity;

    class Scene
    {
    public:
        Scene();
        ~Scene();

        static Ref<Scene> Copy(Ref<Scene> other);

        void UpdateEditor(float dt, EditorCamera& camera);
        void UpdatePlay(float dt);
        void UpdateRunTime(float dt);

        bool IsRunning() const { return mIsRunning; }

        Entity CreateEntity(const std::string& tagName = "Object");
        Entity CreateEntityWithUUID(UUID uuid, const std::string& tagName = "Object");
        void RemoveEntity(Entity entity);

        Entity GetEntity(UUID uuid);

        void DuplicateEntity(Entity entity);

        void RuntimeStart();
        void RuntimeStop();

        void ViewportResize(uint32_t width, uint32_t height);

        Entity GetPrimaryCameraEntity();

        template<typename... Components>
        auto GetAllEntitiesWith()
        {
            return mRegistry.view<Components...>();
        }

    private:
        template<typename T>
        void ComponentAdded(Entity entity, T& component);

    private:
        entt::registry mRegistry;

        uint32_t mViewportWidth = 0, mViewportHeight = 0;

        b2World* mPhysicsWorld = nullptr;
        std::unordered_map<UUID, entt::entity> mEntities;

        int32_t mVelocityIterations = 6, mPositionIterations = 2;

        friend class Entity;
        friend class SceneHierarchyPanel;
        friend class SceneSerializer;

        bool mIsRunning = false;
    };
}