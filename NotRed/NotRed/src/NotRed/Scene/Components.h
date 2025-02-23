#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <set>

#include "NotRed/Core/UUID.h"
#include "NotRed/Renderer/Texture.h"
#include "NotRed/Renderer/Mesh.h"
#include "NotRed/Renderer/MaterialAsset.h"
#include "NotRed/Scene/SceneCamera.h"
#include "NotRed/Renderer/SceneEnvironment.h"
#include "NotRed/Asset/Asset.h"
#include "NotRed/Math/Math.h"
#include "NotRed/Renderer/UI/Font.h"

#include "NotRed/Script/ScriptModuleField.h"

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

    struct PrefabComponent
    {
        UUID PrefabID = 0;
        UUID EntityID = 0;
        PrefabComponent() = default;
        PrefabComponent(const PrefabComponent& other) = default;
    };

    struct TransformComponent
    {
        glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
        glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
        glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

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

        void SetTransform(const glm::mat4& transform) 
        {
            // WARNING:  sets only the translation for now
            Translation = transform[3];
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
        AssetHandle MeshHandle;
        uint32_t SubmeshIndex = 0;
        Ref<MaterialTable> Materials = Ref<MaterialTable>::Create();

        MeshComponent() = default;

        MeshComponent(const MeshComponent& other)
            : MeshHandle(other.MeshHandle), SubmeshIndex(other.SubmeshIndex),
            Materials(Ref<MaterialTable>::Create(other.Materials)) {}

        MeshComponent(AssetHandle mesh, uint32_t submeshIndex = 0)
            : MeshHandle(mesh), SubmeshIndex(submeshIndex) {}
    };

    struct ParticleComponent
    {
        // TODO: Change it to a AssetHandler
        Ref<Mesh> MeshObj;

        int ParticleCount = 80128;
        float Velocity = 1.0f;

        glm::vec3 StarColor = glm::vec3(1.0f, 1.0f, 1.0f);
        glm::vec3 DustColor = glm::vec3(0.388f, 0.333f, 1.0f);
        glm::vec3 h2RegionColor = glm::vec3(0.8f, 0.071f, 0.165f);

        ParticleComponent()
        {
            MeshObj = Ref<Mesh>::Create(Ref<MeshSource>::Create(ParticleCount));
        }
        ParticleComponent(const ParticleComponent& other) = default;
        ParticleComponent(const int particleNum)
        {
            ParticleCount = particleNum;
            MeshObj = Ref<Mesh>::Create(Ref<MeshSource>::Create(particleNum));
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

    struct PointLightComponent
    {
        glm::vec3 Radiance = { 1.0f, 1.0f, 1.0f };
        float Intensity = 1.0f;
        float LightSize = 0.5f; // For PCSS
        float MinRadius = 1.f;
        float Radius = 10.f;
        bool CastsShadows = true;
        bool SoftShadows = true;
        float Falloff = 1.f;
    };

    struct SkyLightComponent
    {
        AssetHandle SceneEnvironment;
        float Intensity = 1.0f;
        float Lod = 0.0f;

        bool DynamicSky = false;
        glm::vec3 TurbidityAzimuthInclination = { 2.0, 0.0, 0.0 };
    };

    struct AnimationComponent
    {
        AssetHandle AnimationController;
    };

    struct TextComponent
    {
        std::string TextString = "Text";

        // Font
        AssetHandle FontAsset;
        glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
        float LineSpacing = 0.0f;
        float Kerning = 0.0f;
        
        // Layout
        float MaxWidth = 10.0f;
        TextComponent() = default;
        TextComponent(const TextComponent& other) = default;
    };

    struct StaticMeshComponent
    {
        AssetHandle StaticMesh;
        Ref<MaterialTable> Materials = Ref<MaterialTable>::Create();
        StaticMeshComponent() = default;
        StaticMeshComponent(const StaticMeshComponent& other)
            : StaticMesh(other.StaticMesh), Materials(Ref<MaterialTable>::Create(other.Materials))
        { }

        StaticMeshComponent(AssetHandle staticMesh)
            : StaticMesh(staticMesh) {}
    };

    struct ScriptComponent
    {
        std::string ModuleName;
        ScriptModuleFieldMap ModuleFieldMap;

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
        AssetHandle Texture;
        float TilingFactor = 1.0f;

        SpriteRendererComponent() = default;
        SpriteRendererComponent(const SpriteRendererComponent& other) = default;
    };

    struct RigidBody2DComponent
    {
        enum class Type { Static, Dynamic, Kinematic };
        Type BodyType = Type::Static;
        bool FixedRotation = false;
        float Mass = 1.0f;
        float LinearDrag = 0.01f;
        float AngularDrag = 0.05f;
        float GravityScale = 1.0f;
        bool IsBullet = false;

        void* RuntimeBody = nullptr;

        RigidBody2DComponent() = default;
        RigidBody2DComponent(const RigidBody2DComponent& other) = default;
    };

    struct BoxCollider2DComponent
    {
        glm::vec2 Offset = { 0.0f,0.0f };
        glm::vec2 Size = { 0.5f, 0.5f };

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
        enum class CollisionDetectionType : uint32_t { Discrete, Continuous, ContinuousSpeculative };

        Type BodyType = Type::Dynamic;
        float Mass = 1.0f;
        float LinearDrag = 0.01f;
        float AngularDrag = 0.05f;
        bool DisableGravity = false;
        bool IsKinematic = false;
        uint32_t Layer = 0;

        CollisionDetectionType CollisionDetection = CollisionDetectionType::Discrete;

        bool LockPositionX = false;
        bool LockRotationX = false;

        bool LockPositionY = false;
        bool LockRotationY = false;

        bool LockPositionZ = false;
        bool LockRotationZ = false;

        RigidBodyComponent() = default;
        RigidBodyComponent(const RigidBodyComponent& other) = default;
    };

    struct CharacterControllerComponent
    {
        float SlopeLimit;
        float StepOffset;
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
        AssetHandle Material;

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
        AssetHandle Material;

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
        AssetHandle Material;

        Ref<Mesh> DebugMesh;

        CapsuleColliderComponent() = default;
        CapsuleColliderComponent(const CapsuleColliderComponent& other) = default;
    };

    struct MeshColliderComponent
    {
        static constexpr ColliderComponentType Type = ColliderComponentType::MeshCollider;

        AssetHandle CollisionMesh;
        uint32_t SubmeshIndex = 0;
        bool OverrideMesh = false;

        bool IsTrigger = false;
        AssetHandle Material;

        MeshColliderComponent() = default;
        MeshColliderComponent(const MeshColliderComponent& other) = default;
        MeshColliderComponent(AssetHandle mesh, uint32_t submeshIndex = 0)
            : CollisionMesh(mesh), SubmeshIndex(submeshIndex)
        {}
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

    template<typename... Component>
    struct ComponentGroup {};

    using AllComponents =
        ComponentGroup<IDComponent, TagComponent, TransformComponent, RelationshipComponent, 
        MeshComponent, ParticleComponent, 
        PointLightComponent, SkyLightComponent, 
        ScriptComponent, CameraComponent, SpriteRendererComponent, 
        RigidBody2DComponent, BoxCollider2DComponent, CircleCollider2DComponent, RigidBodyComponent, 
        BoxColliderComponent, SphereColliderComponent, CapsuleColliderComponent, MeshColliderComponent, 
        AudioListenerComponent>;
}