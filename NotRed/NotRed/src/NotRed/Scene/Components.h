#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "SceneCamera.h"
#include "ScriptableEntity.h"
#include "NotRed/Renderer/Texture.h"

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
        glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
        glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
        glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const glm::vec3& translation)
            : Translation(translation) {};

        glm::mat4 GetTransform() const
        {
            glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));

            return glm::translate(glm::mat4(1.0f), Translation)
                * rotation
                * glm::scale(glm::mat4(1.0f), Scale);
        }
    };

    struct SpriteRendererComponent
    {
        glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
        Ref<Texture2D> Texture;

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

        void (*InstantiateFunction)() = nullptr;
        void (*DestroyInstanceFunction)() = nullptr;

        void (*CreateFunction)() = {};
        void (*UpdateFunction)(float) = {};
        void (*DestroyFunction) = {};

        template<typename T>
        void Bind()
        {
            InstantiateFunction = [&]() { Instance = new T(); };
            DestroyInstanceFunction = [&]() { delete (T*)Instance; Instance = nullptr; };

            if constexpr (std::is_invocable_v<decltype(&T::Create), T>)
            {
                CreateFunction = [&]() { static_cast<T*>(Instance)->Create(); };
            }

            if constexpr (std::is_invocable_v<decltype(&T::Update), T, float>)
            {
                UpdateFunction = [&](float dt) { static_cast<T*>(Instance)->Update(dt); };
            }

            if constexpr (std::is_invocable_v<decltype(&T::Destroy), T>)
            {
                DestroyFunction = [&]() { static_cast<T*>(Instance)->Destroy(); };
            }
        }
    };
}