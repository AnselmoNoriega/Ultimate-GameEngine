#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <string>

#include "SceneCamera.h"
#include "NotRed/Core/UUID.h"

#include "Components2D.h"
#include "Components3D.h"

namespace NR
{
    struct IDComponent
    {
        UUID ID;

        IDComponent(UUID uuid) { ID = uuid; }
        IDComponent(const IDComponent&) = default;
    };

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

    class ScriptableEntity;
    struct NativeScriptComponent
    {
        ScriptableEntity* Instance = nullptr;

        void (*InstantiateFunction)() = nullptr;
        void (*DestroyInstanceFunction)() = nullptr;

        void (*CreateFunction)() = {};
        void (*UpdateFunction)(float) = {};
        void(*DestroyFunction) = {};

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

    struct ScriptComponent
    {
        std::string ClassName;

        ScriptComponent() = default;
        ScriptComponent(const ScriptComponent&) = default;
    };

    struct CameraComponent
    {
        SceneCamera Camera;
        bool IsPrimary = true;
        bool HasFixedAspectRatio = false;

        CameraComponent() = default;
        CameraComponent(const CameraComponent&) = default;
    };

    template<typename... Component>
    struct ComponentGroup {};

    using AllComponents =
        ComponentGroup<TransformComponent, SpriteRendererComponent,
        CircleRendererComponent, CameraComponent, NativeScriptComponent,
        ScriptComponent, Rigidbody2DComponent, BoxCollider2DComponent,
        CircleCollider2DComponent, TextComponent>;
}