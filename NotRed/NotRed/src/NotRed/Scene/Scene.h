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
        void RemoveEntity(Entity entity);

        void ViewportResize(uint32_t width, uint32_t height);

    private:
        template<typename T>
        void ComponentAdded(Entity entity, T& component);

    private:
        entt::registry mRegistry;

        friend class Entity;
        friend class SceneHierarchyPanel;
        friend class SceneSerializer;

        uint32_t mWidth = 0, mHeight = 0;
    };
}