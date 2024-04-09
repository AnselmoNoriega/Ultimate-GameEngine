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

        void UpdateEditor(float dt, EditorCamera& camera);
        void UpdateRunTime(float dt);

        Entity CreateEntity(const std::string& tagName = "Object");
        Entity CreateEntityWithUUID(UUID uuid, const std::string& tagName = "Object");
        void RemoveEntity(Entity entity);

        void RuntimeStart();
        void RuntimeStop();

        void ViewportResize(uint32_t width, uint32_t height);

        Entity GetPrimaryCameraEntity();

    private:
        template<typename T>
        void ComponentAdded(Entity entity, T& component);

    private:
        entt::registry mRegistry;

        uint32_t mViewportWidth = 0, mViewportHeight = 0;

        b2World* mPhysicsWorld = nullptr;

        int32_t mVelocityIterations = 6, mPositionIterations = 2;

        friend class Entity;
        friend class SceneHierarchyPanel;
        friend class SceneSerializer;
    };
}