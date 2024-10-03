#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "NotRed/Core/UUID.h"
#include "NotRed/Renderer/Texture.h"
#include "NotRed/Renderer/Mesh.h"
#include "NotRed/Scene/SceneCamera.h"
#include "NotRed/Renderer/SceneEnvironment.h"
#include "NotRed/Asset/Asset.h"

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
        glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
        glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
        glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

        glm::vec3 Up = { 0.0f, 1.0f, 0.0f };
        glm::vec3 Right = { 1.0f, 0.0f, 0.0f };
        glm::vec3 Forward = { 0.0f, 0.0f, -1.0f };

        TransformComponent() = default;
        TransformComponent(const TransformComponent& other) = default;
        TransformComponent(const glm::vec3& translation)
            : Translation(translation) {}

        glm::mat4 GetTransform() const
        {
            return glm::translate(glm::mat4(1.0f), Translation)
                * glm::toMat4(glm::quat(glm::radians(Rotation)))
                * glm::scale(glm::mat4(1.0f), Scale);
        }
    };

    struct RelationshipComponent
    {
        UUID ParentHandle = 0;

        std::vector<UUID> Children;

        RelationshipComponent() = default;
        RelationshipComponent(const RelationshipComponent& other) = default;
        RelationshipComponent(UUID parent)
            : ParentHandle(parent) {}
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

    struct ParticleComponent
    {
        Ref<Mesh> MeshObj;

        int ParticleCount = 1;

        ParticleComponent()
        {
            MeshObj = Ref<Mesh>::Create(ParticleCount);
        }
        ParticleComponent(const ParticleComponent& other) = default;
        ParticleComponent(const int particleNum)
        {
            ParticleCount = particleNum;
            MeshObj = Ref<Mesh>::Create(particleNum);
        }

        operator Ref<Mesh>() { return MeshObj; }
    };

    enum class LightType
    {
        None, Directional, Point, Spot
    };

    struct DirectionalLightComponent
    {
        glm::vec3 Radiance = { 1.0f, 1.0f, 1.0f };
        float Intensity = 1.0f;
        bool CastShadows = true;
        bool SoftShadows = true;
        float LightSize = 0.5f; // For PCSS
    };

    struct SkyLightComponent
    {
        Ref<Environment> SceneEnvironment;
        float Intensity = 1.0f;
        float Angle = 0.0f;

        bool DynamicSky = false;
        glm::vec3 TurbidityAzimuthInclination = { 2.0, 0.0, 0.0 };
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
        Type BodyType = Type::Static;
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
        float LinearDrag = 0.0f;
        float AngularDrag = 0.05f;
        bool DisableGravity = false;
        bool IsKinematic = false;
        uint32_t Layer = 0;

        bool LockPositionX = false;
        bool LockRotationX = false;

        bool LockPositionY = false;
        bool LockRotationY = false;

        bool LockPositionZ = false;
        bool LockRotationZ = false;

        RigidBodyComponent() = default;
        RigidBodyComponent(const RigidBodyComponent& other) = default;
    };


    enum class ColliderComponentType
    {
        BoxCollider, 
        SphereCollider, 
        CapsuleCollider, 
        MeshCollider
    };

    struct BoxColliderComponent
    {
        static constexpr ColliderComponentType Type = ColliderComponentType::BoxCollider;

        glm::vec3 Size = { 1.0f, 1.0f, 1.0f };
        glm::vec3 Offset = { 0.0f, 0.0f, 0.0f };

        bool IsTrigger = false;
        Ref<PhysicsMaterial> Material;

        Ref<Mesh> DebugMesh;

        BoxColliderComponent() = default;
        BoxColliderComponent(const BoxColliderComponent& other) = default;
    };

    struct SphereColliderComponent
    {
        static constexpr ColliderComponentType Type = ColliderComponentType::SphereCollider;

        float Radius = 0.5f;
        glm::vec3 Offset = glm::vec3(0.0f);

        bool IsTrigger = false;
        Ref<PhysicsMaterial> Material;

        Ref<Mesh> DebugMesh;

        SphereColliderComponent() = default;
        SphereColliderComponent(const SphereColliderComponent& other) = default;
    };

    struct CapsuleColliderComponent
    {
        static constexpr ColliderComponentType Type = ColliderComponentType::CapsuleCollider;

        float Radius = 0.5f;
        float Height = 1.0f;
        glm::vec3 Offset = glm::vec3(0.0f);

        bool IsTrigger = false;
        Ref<PhysicsMaterial> Material;

        Ref<Mesh> DebugMesh;

        CapsuleColliderComponent() = default;
        CapsuleColliderComponent(const CapsuleColliderComponent& other) = default;
    };

    struct MeshColliderComponent
    {
        static constexpr ColliderComponentType Type = ColliderComponentType::MeshCollider;

        Ref<Mesh> CollisionMesh;
        std::vector<Ref<Mesh>> ProcessedMeshes;
        bool IsConvex = false;
        bool OverrideMesh = false;

        bool IsTrigger = false;
        Ref<PhysicsMaterial> Material;

        MeshColliderComponent() = default;
        MeshColliderComponent(const MeshColliderComponent& other) = default;
        MeshColliderComponent(const Ref<Mesh>& mesh)
            : CollisionMesh(mesh)
        {
        }

        operator Ref<Mesh>() { return CollisionMesh; }
    };

    struct AudioListenerComponent
    {
        bool Active = false;

        float ConeInnerAngleInRadians = 6.283185f;
        float ConeOuterAngleInRadians = 6.283185f;

        float ConeOuterGain = 0.0f;

        AudioListenerComponent() = default;
        AudioListenerComponent(const AudioListenerComponent& other) = default;
    };
}