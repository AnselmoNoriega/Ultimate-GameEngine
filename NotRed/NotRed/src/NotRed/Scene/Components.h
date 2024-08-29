#pragma once

#include <glm/glm.hpp>

#include "NotRed/Core/UUID.h"
#include "NotRed/Renderer/Texture.h"
#include "NotRed/Renderer/Mesh.h"
#include "NotRed/Scene/SceneCamera.h"

namespace NR
{
    struct IDComponent
    {
        UUID ID = 0;
    };

    struct TagComponent
    {
        std::string Tag;

        TagComponent() = default;
        TagComponent(const TagComponent& other) = default;
        TagComponent(const std::string& tag)
            : Tag(tag) {}

        operator std::string& () { return Tag; }
        operator const std::string& () const { return Tag; }
    };

    struct TransformComponent
    {
        glm::mat4 Transform;

        TransformComponent() = default;
        TransformComponent(const TransformComponent& other) = default;
        TransformComponent(const glm::mat4& transform)
            : Transform(transform) {}

        operator glm::mat4& () { return Transform; }
        operator const glm::mat4& () const { return Transform; }
    };

    struct MeshComponent
    {
        Ref<Mesh> MeshObj;

        MeshComponent() = default;
        MeshComponent(const MeshComponent& other) = default;
        MeshComponent(const Ref<Mesh>& mesh)
            : MeshObj(mesh) {}

        operator Ref<Mesh>() { return MeshObj; }
    };

    struct ScriptComponent
    {
        std::string ModuleName;

        ScriptComponent() = default;
        ScriptComponent(const ScriptComponent& other) = default;
        ScriptComponent(const std::string& moduleName)
            : ModuleName(moduleName) {}
    };

    struct CameraComponent
    {
        SceneCamera CameraObj;
        bool Primary = true;

        CameraComponent() = default;
        CameraComponent(const CameraComponent& other) = default;

        operator SceneCamera& () { return CameraObj; }
        operator const SceneCamera& () const { return CameraObj; }
    };

    struct SpriteRendererComponent
    {
        glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
        Ref<Texture2D> Texture;
        float TilingFactor = 1.0f;

        SpriteRendererComponent() = default;
        SpriteRendererComponent(const SpriteRendererComponent& other) = default;
    };

    struct RigidBody2DComponent
    {
        enum class Type { Static, Dynamic, Kinematic };
        Type BodyType;
        bool FixedRotation = false;

        void* RuntimeBody = nullptr;

        RigidBody2DComponent() = default;
        RigidBody2DComponent(const RigidBody2DComponent& other) = default;
    };

    struct BoxCollider2DComponent
    {
        glm::vec2 Offset = { 0.0f,0.0f };
        glm::vec2 Size = { 1.0f, 1.0f };

        float Density = 1.0f;
        float Friction = 1.0f;

        void* RuntimeFixture = nullptr;

        BoxCollider2DComponent() = default;
        BoxCollider2DComponent(const BoxCollider2DComponent& other) = default;
    };

    struct CircleCollider2DComponent
    {
        glm::vec2 Offset = { 0.0f,0.0f };
        float Radius = 1.0f;

        float Density = 1.0f;
        float Friction = 1.0f;

        void* RuntimeFixture = nullptr;

        CircleCollider2DComponent() = default;
        CircleCollider2DComponent(const CircleCollider2DComponent& other) = default;
    };

    struct RigidBodyComponent
    {
        enum class Type { Static, Dynamic };

        Type BodyType;
        float Mass = 1.0f;
        bool IsKinematic = false;

        bool LockPositionX = false;
        bool LockRotationX = false;

        bool LockPositionY = false;
        bool LockRotationY = false;

        bool LockPositionZ = false;
        bool LockRotationZ = false;

        void* RuntimeActor = nullptr;

        RigidBodyComponent() = default;
        RigidBodyComponent(const RigidBodyComponent& other) = default;
    };

    struct PhysicsMaterialComponent
    {
        float StaticFriction = 1.0f;
        float DynamicFriction = 1.0f;
        float Bounciness = 1.0f;

        PhysicsMaterialComponent() = default;
        PhysicsMaterialComponent(const PhysicsMaterialComponent& other) = default;
    };

    struct BoxColliderComponent
    {
        glm::vec3 Size = { 1.0f, 1.0f, 1.0f };
        glm::vec3 Offset = { 0.0f, 0.0f, 0.0f };

        BoxColliderComponent() = default;
        BoxColliderComponent(const BoxColliderComponent& other) = default;
    };

    struct SphereColliderComponent
    {
        float Radius;
        glm::vec3 Offset = { 0.0f, 0.0f, 0.0f };

        SphereColliderComponent() = default;
        SphereColliderComponent(const SphereColliderComponent& other) = default;
    };

    struct CapsuleColliderComponent
    {
        float Radius = 0.5f;
        float Height = 1.0f;
        glm::vec3 Offset = { 0.0f, 0.0f, 0.0f };

        CapsuleColliderComponent() = default;
        CapsuleColliderComponent(const CapsuleColliderComponent& other) = default;
    };

    struct MeshColliderComponent
    {
        Ref<Mesh> CollisionMesh;

        MeshColliderComponent() = default;
        MeshColliderComponent(const MeshColliderComponent& other) = default;
        MeshColliderComponent(const Ref<Mesh>& mesh)
            : CollisionMesh(mesh)
        {
        }

        operator Ref<Mesh>() { return CollisionMesh; }
    };
}