#pragma once

#include <glm/glm.hpp>

#include "SceneCamera.h"
#include "ScriptableEntity.h"

namespace NR
{
    struct TagComponent
    {
        std::string Tag;

        TagComponent() = default;
        TagComponent(const TagComponent&) = default;
        TagComponent(const std::string& tag)
            : Tag(tag) {};

        operator std::string& () { return Tag; }
        operator const std::string& () const { return Tag; }
        operator const char* () const { return Tag.c_str(); }
    };

    struct TransformComponent
    {
        glm::mat4 Transform = glm::mat4(1.0f);

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const glm::mat4& transform)
            : Transform(transform) {};

        operator glm::mat4& () { return Transform; }
        operator const glm::mat4& () const { return Transform; }
    };

    struct SpriteRendererComponent
    {
        glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };

        SpriteRendererComponent() = default;
        SpriteRendererComponent(const SpriteRendererComponent&) = default;
        SpriteRendererComponent(const glm::vec4& color)
            : Color(color) {};
    };

    struct CameraComponent
    {
        SceneCamera Camera;
        bool IsPrimary = true;
        bool HasFixedAspectRatio = false;

        CameraComponent() = default;
        CameraComponent(const CameraComponent&) = default;
    };

    struct NativeScriptComponent
    {
        ScriptableEntity* Instance = nullptr;

        std::function<void()> InstantiateFunction;
        std::function<void()> DestroyInstanceFunction;

        std::function<void()> CreateFunction;
        std::function<void(float)> UpdateFunction;
        std::function<void()> DestroyFunction;

        template<typename T>
        void Bind()
        {
            InstantiateFunction = [&]() { Instance = new T(); };
            DestroyInstanceFunction = [&]() { delete (T*)Instance; Instance = nullptr; };

            CreateFunction = [&]() { ((T*)Instance)->Create(); };
            UpdateFunction = [&](float dt) { ((T*)Instance)->Update(); };
            DestroyFunction = [&]() { ((T*)Instance)->Destroy(); };
        }
    };
}