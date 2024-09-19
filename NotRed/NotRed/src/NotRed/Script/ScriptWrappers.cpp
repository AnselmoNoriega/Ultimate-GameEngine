#include "nrpch.h"
#include "ScriptWrappers.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/common.hpp>

#include <mono/jit/jit.h>

#include <box2d/box2d.h>

#include <PxPhysicsAPI.h>
#include "NotRed/Physics/PhysicsUtil.h"
#include "NotRed/Physics/PhysicsWrappers.h"
#include "NotRed/Physics/PhysicsActor.h"

#include "NotRed/Math/Noise.h"

#include "NotRed/Scene/Scene.h"
#include "NotRed/Scene/Entity.h"

namespace NR
{
    extern std::unordered_map<MonoType*, std::function<bool(Entity&)>> sHasComponentFuncs;
    extern std::unordered_map<MonoType*, std::function<void(Entity&)>> sCreateComponentFuncs;
}

namespace NR::Script
{
    ////////////////////////////////////////////////////////////////
    // Math ////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////

    float NR_Noise_PerlinNoise(float x, float y)
    {
        return Noise::PerlinNoise(x, y);
    }

    ////////////////////////////////////////////////////////////////
    // Input ///////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////

    bool NR_Input_IsKeyPressed(KeyCode key)
    {
        return Input::IsKeyPressed(key);
    }

    bool NR_Input_IsMouseButtonPressed(MouseButton button)
    {
        return Input::IsMouseButtonPressed(button);
    }

    ////////////////////////////////////////////////////////////////
    // Entity //////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////

    void NR_Entity_CreateComponent(uint64_t entityID, void* type)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        MonoType* monoType = mono_reflection_type_get_type((MonoReflectionType*)type);
        sCreateComponentFuncs[monoType](entity);
    }

    bool NR_Entity_HasComponent(uint64_t entityID, void* type)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        MonoType* monoType = mono_reflection_type_get_type((MonoReflectionType*)type);
        bool result = sHasComponentFuncs[monoType](entity);
        return result;
    }

    uint64_t NR_Entity_FindEntityByTag(MonoString* tag)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        Entity entity = scene->FindEntityByTag(mono_string_to_utf8(tag));
        if (entity)
        {
            return entity.GetComponent<IDComponent>().ID;
        }

        return 0;
    }

    void NR_TransformComponent_GetTransform(uint64_t entityID, TransformComponent* outTransform)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        *outTransform = entity.GetComponent<TransformComponent>();
    }

    void NR_TransformComponent_SetTransform(uint64_t entityID, TransformComponent* inTransform)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        entity.GetComponent<TransformComponent>() = *inTransform;
    }

    void NR_TransformComponent_GetTranslation(uint64_t entityID, glm::vec3* outTranslation)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        *outTranslation = entity.GetComponent<TransformComponent>().Translation;
    }

    void NR_TransformComponent_SetTranslation(uint64_t entityID, glm::vec3* inTranslation)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        entity.GetComponent<TransformComponent>().Translation = *inTranslation;
    }

    void NR_TransformComponent_GetRotation(uint64_t entityID, glm::vec3* outRotation)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        *outRotation = entity.GetComponent<TransformComponent>().Rotation;
    }

    void NR_TransformComponent_SetRotation(uint64_t entityID, glm::vec3* inRotation)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        entity.GetComponent<TransformComponent>().Rotation = *inRotation;
    }

    void NR_TransformComponent_GetScale(uint64_t entityID, glm::vec3* outScale)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        *outScale = entity.GetComponent<TransformComponent>().Scale;
    }

    void NR_TransformComponent_SetScale(uint64_t entityID, glm::vec3* inScale)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        entity.GetComponent<TransformComponent>().Scale = *inScale;
    }

    void NR_Input_GetMousePosition(glm::vec2* outPosition)
    {
        auto [x, y] = Input::GetMousePosition();
        *outPosition = { x, y };
    }

    void NR_Input_SetCursorMode(CursorMode mode)
    {
        Input::SetCursorMode(mode);
    }

    CursorMode NR_Input_GetCursorMode()
    {
        return Input::GetCursorMode();
    }

    bool NR_Physics_Raycast(glm::vec3* origin, glm::vec3* direction, float maxDistance, RaycastHit* hit)
    {
        return PhysicsWrappers::Raycast(*origin, *direction, maxDistance, hit);
    }

    static void AddCollidersToArray(MonoArray* array, const std::array<physx::PxOverlapHit, OVERLAP_MAX_COLLIDERS>& hits, uint32_t count, uint32_t arrayLength)
    {
        uint32_t arrayIndex = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            Entity& entity = *(Entity*)hits[i].actor->userData;

            if (entity.HasComponent<BoxColliderComponent>() && arrayIndex < arrayLength)
            {
                auto& boxCollider = entity.GetComponent<BoxColliderComponent>();

                void* data[] = {
                    &entity.GetID(),
                    &boxCollider.Size,
                    &boxCollider.Offset,
                    &boxCollider.IsTrigger
                };

                MonoObject* obj = ScriptEngine::Construct("NR.BoxCollider:.ctor(ulong,Vector3,Vector3,bool)", true, data);
                mono_array_set(array, MonoObject*, arrayIndex++, obj);
            }

            if (entity.HasComponent<SphereColliderComponent>() && arrayIndex < arrayLength)
            {
                auto& sphereCollider = entity.GetComponent<SphereColliderComponent>();

                void* data[] = {
                    &entity.GetID(),
                    &sphereCollider.Radius,
                    &sphereCollider.IsTrigger
                };

                MonoObject* obj = ScriptEngine::Construct("NR.SphereCollider:.ctor(ulong,single,bool)", true, data);
                mono_array_set(array, MonoObject*, arrayIndex++, obj);
            }

            if (entity.HasComponent<CapsuleColliderComponent>() && arrayIndex < arrayLength)
            {
                auto& capsuleCollider = entity.GetComponent<CapsuleColliderComponent>();

                void* data[] = {
                    &entity.GetID(),
                    &capsuleCollider.Radius,
                    &capsuleCollider.Height,
                    &capsuleCollider.IsTrigger
                };

                MonoObject* obj = ScriptEngine::Construct("NR.CapsuleCollider:.ctor(ulong,single,single,bool)", true, data);
                mono_array_set(array, MonoObject*, arrayIndex++, obj);
            }

            if (entity.HasComponent<MeshColliderComponent>() && arrayIndex < arrayLength)
            {
                auto& meshCollider = entity.GetComponent<MeshColliderComponent>();

                Ref<Mesh>* mesh = new Ref<Mesh>(meshCollider.CollisionMesh);
                void* data[] = {
                    &entity.GetID(),
                    &mesh,
                    &meshCollider.IsTrigger
                };

                MonoObject* obj = ScriptEngine::Construct("NR.MeshCollider:.ctor(ulong,intptr,bool)", true, data);
                mono_array_set(array, MonoObject*, arrayIndex++, obj);
            }
        }

    }

    static std::array<physx::PxOverlapHit, OVERLAP_MAX_COLLIDERS> sOverlapBuffer;

    MonoArray* NR_Physics_OverlapBox(glm::vec3* origin, glm::vec3* halfSize)
    {
        MonoArray* outColliders = nullptr;
        memset(sOverlapBuffer.data(), 0, OVERLAP_MAX_COLLIDERS * sizeof(physx::PxOverlapHit));

        uint32_t count;
        if (PhysicsWrappers::OverlapBox(*origin, *halfSize, sOverlapBuffer, &count))
        {
            outColliders = mono_array_new(mono_domain_get(), ScriptEngine::GetCoreClass("NR.Collider"), count);
            AddCollidersToArray(outColliders, sOverlapBuffer, count, count);
        }

        return outColliders;
    }

    MonoArray* NR_Physics_OverlapCapsule(glm::vec3* origin, float radius, float halfHeight)
    {
        MonoArray* outColliders = nullptr;
        memset(sOverlapBuffer.data(), 0, OVERLAP_MAX_COLLIDERS * sizeof(physx::PxOverlapHit));

        uint32_t count;
        if (PhysicsWrappers::OverlapCapsule(*origin, radius, halfHeight, sOverlapBuffer, &count))
        {
            outColliders = mono_array_new(mono_domain_get(), ScriptEngine::GetCoreClass("NR.Collider"), count);
            AddCollidersToArray(outColliders, sOverlapBuffer, count, count);
        }
        return outColliders;
    }

    int32_t NR_Physics_OverlapBoxNonAlloc(glm::vec3* origin, glm::vec3* halfSize, MonoArray* outColliders)
    {
        memset(sOverlapBuffer.data(), 0, OVERLAP_MAX_COLLIDERS * sizeof(physx::PxOverlapHit));

        uint64_t arrayLength = mono_array_length(outColliders);

        uint32_t count;
        if (PhysicsWrappers::OverlapBox(*origin, *halfSize, sOverlapBuffer, &count))
        {
            if (count > arrayLength)
            {
                count = arrayLength;
            }

            AddCollidersToArray(outColliders, sOverlapBuffer, count, arrayLength);
        }

        return count;
    }

    int32_t NR_Physics_OverlapCapsuleNonAlloc(glm::vec3* origin, float radius, float halfHeight, MonoArray* outColliders)
    {
        memset(sOverlapBuffer.data(), 0, OVERLAP_MAX_COLLIDERS * sizeof(physx::PxOverlapHit));

        uint64_t arrayLength = mono_array_length(outColliders);

        uint32_t count;
        if (PhysicsWrappers::OverlapCapsule(*origin, radius, halfHeight, sOverlapBuffer, &count))
        {
            if (count > arrayLength)
            {
                count = arrayLength;
            }

            AddCollidersToArray(outColliders, sOverlapBuffer, count, arrayLength);
        }

        return count;
    }

    int32_t NR_Physics_OverlapSphereNonAlloc(glm::vec3* origin, float radius, MonoArray* outColliders)
    {
        memset(sOverlapBuffer.data(), 0, OVERLAP_MAX_COLLIDERS * sizeof(physx::PxOverlapHit));

        uint64_t arrayLength = mono_array_length(outColliders);

        uint32_t count;
        if (PhysicsWrappers::OverlapSphere(*origin, radius, sOverlapBuffer, &count))
        {
            if (count > arrayLength)
            {
                count = arrayLength;
            }

            AddCollidersToArray(outColliders, sOverlapBuffer, count, arrayLength);
        }

        return count;
    }

    MonoArray* NR_Physics_OverlapSphere(glm::vec3* origin, float radius)
    {
        MonoArray* outColliders = nullptr;
        memset(sOverlapBuffer.data(), 0, OVERLAP_MAX_COLLIDERS * sizeof(physx::PxOverlapHit));

        uint32_t count;
        if (PhysicsWrappers::OverlapSphere(*origin, radius, sOverlapBuffer, &count))
        {
            outColliders = mono_array_new(mono_domain_get(), ScriptEngine::GetCoreClass("NR.Collider"), count);
            AddCollidersToArray(outColliders, sOverlapBuffer, count, count);
        }

        return outColliders;
    }

    void* NR_MeshComponent_GetMesh(uint64_t entityID)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        auto& meshComponent = entity.GetComponent<MeshComponent>();
        return new Ref<Mesh>(meshComponent.MeshObj);
    }

    void NR_MeshComponent_SetMesh(uint64_t entityID, Ref<Mesh>* inMesh)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        auto& meshComponent = entity.GetComponent<MeshComponent>();
        meshComponent.MeshObj = inMesh ? *inMesh : nullptr;
    }

    void NR_RigidBody2DComponent_ApplyLinearImpulse(uint64_t entityID, glm::vec2* impulse, glm::vec2* offset, bool wake)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<RigidBody2DComponent>());
        auto& component = entity.GetComponent<RigidBody2DComponent>();
        b2Body* body = (b2Body*)component.RuntimeBody;
        body->ApplyLinearImpulse(*(const b2Vec2*)impulse, body->GetWorldCenter() + *(const b2Vec2*)offset, wake);
    }

    void NR_RigidBody2DComponent_GetLinearVelocity(uint64_t entityID, glm::vec2* outVelocity)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        auto& component = entity.GetComponent<RigidBody2DComponent>();
        b2Body* body = (b2Body*)component.RuntimeBody;
        const auto& velocity = body->GetLinearVelocity();
        NR_CORE_ASSERT(outVelocity);
        *outVelocity = { velocity.x, velocity.y };
    }

    void NR_RigidBody2DComponent_SetLinearVelocity(uint64_t entityID, glm::vec2* velocity)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        auto& component = entity.GetComponent<RigidBody2DComponent>();
        b2Body* body = (b2Body*)component.RuntimeBody;
        NR_CORE_ASSERT(velocity);
        body->SetLinearVelocity({ velocity->x, velocity->y });
    }

    RigidBodyComponent::Type NR_RigidBodyComponent_GetBodyType(uint64_t entityID)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
        auto& component = entity.GetComponent<RigidBodyComponent>();
        return component.BodyType;
    }

    void NR_RigidBodyComponent_AddForce(uint64_t entityID, glm::vec3* force, ForceMode forceMode)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
        auto& component = entity.GetComponent<RigidBodyComponent>();

        if (component.IsKinematic)
        {
            NR_CORE_WARN("Cannot add a force to a kinematic actor! EntityID({0})", entityID);
            return;
        }

        Ref<PhysicsActor> actor = PhysicsManager::GetActorForEntity(entity);
        actor->AddForce(*force, forceMode);
    }

    void NR_RigidBodyComponent_AddTorque(uint64_t entityID, glm::vec3* torque, ForceMode forceMode)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
        auto& component = entity.GetComponent<RigidBodyComponent>();

        if (component.IsKinematic)
        {
            NR_CORE_WARN("Cannot add torque to a kinematic actor! EntityID({0})", entityID);
            return;
        }

        Ref<PhysicsActor> actor = PhysicsManager::GetActorForEntity(entity);
        actor->AddTorque(*torque, forceMode);
    }

    void NR_RigidBodyComponent_GetLinearVelocity(uint64_t entityID, glm::vec3* outVelocity)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
        auto& component = entity.GetComponent<RigidBodyComponent>();

        NR_CORE_ASSERT(outVelocity);
        Ref<PhysicsActor> actor = PhysicsManager::GetActorForEntity(entity);
        *outVelocity = actor->GetVelocity();
    }

    void NR_RigidBodyComponent_SetLinearVelocity(uint64_t entityID, glm::vec3* velocity)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
        auto& component = entity.GetComponent<RigidBodyComponent>();

        NR_CORE_ASSERT(velocity);
        Ref<PhysicsActor> actor = PhysicsManager::GetActorForEntity(entity);
        actor->SetVelocity(*velocity);
    }

    void NR_RigidBodyComponent_GetAngularVelocity(uint64_t entityID, glm::vec3* outVelocity)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
        auto& component = entity.GetComponent<RigidBodyComponent>();

        NR_CORE_ASSERT(outVelocity);
        Ref<PhysicsActor> actor = PhysicsManager::GetActorForEntity(entity);
        *outVelocity = actor->GetAngularVelocity();
    }

    void NR_RigidBodyComponent_SetAngularVelocity(uint64_t entityID, glm::vec3* velocity)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
        auto& component = entity.GetComponent<RigidBodyComponent>();

        NR_CORE_ASSERT(velocity);
        Ref<PhysicsActor> actor = PhysicsManager::GetActorForEntity(entity);
        actor->SetAngularVelocity(*velocity);
    }

    void NR_RigidBodyComponent_Rotate(uint64_t entityID, glm::vec3* rotation)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
        auto& component = entity.GetComponent<RigidBodyComponent>();

        NR_CORE_ASSERT(rotation);
        Ref<PhysicsActor> actor = PhysicsManager::GetActorForEntity(entity);
        actor->Rotate(*rotation);
    }

    uint32_t NR_RigidBodyComponent_GetLayer(uint64_t entityID)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
        auto& component = entity.GetComponent<RigidBodyComponent>();
        return component.Layer;
    }

    float NR_RigidBodyComponent_GetMass(uint64_t entityID)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
        auto& component = entity.GetComponent<RigidBodyComponent>();

        const Ref<PhysicsActor>& actor = PhysicsManager::GetActorForEntity(entity);
        return actor->GetMass();
    }

    void NR_RigidBodyComponent_SetMass(uint64_t entityID, float mass)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
        auto& component = entity.GetComponent<RigidBodyComponent>();

        Ref<PhysicsActor> actor = PhysicsManager::GetActorForEntity(entity);
        actor->SetMass(mass);
    }

    Ref<Mesh>* NR_Mesh_Constructor(MonoString* filepath)
    {
        return new Ref<Mesh>(new Mesh(mono_string_to_utf8(filepath)));
    }

    void NR_Mesh_Destructor(Ref<Mesh>* _this)
    {
        Ref<Mesh>* instance = (Ref<Mesh>*)_this;
        delete _this;
    }

    Ref<Material>* NR_Mesh_GetMaterial(Ref<Mesh>* inMesh)
    {
        Ref<Mesh>& mesh = *(Ref<Mesh>*)inMesh;
        const auto& materials = mesh->GetMaterials();
        return new Ref<Material>(materials[0]);
    }

    Ref<Material>* NR_Mesh_GetMaterialByIndex(Ref<Mesh>* inMesh, int index)
    {
        Ref<Mesh>& mesh = *(Ref<Mesh>*)inMesh;
        const auto& materials = mesh->GetMaterials();

        NR_CORE_ASSERT(index < materials.size());
        return new Ref<Material>(materials[index]);
    }

    int NR_Mesh_GetMaterialCount(Ref<Mesh>* inMesh)
    {
        Ref<Mesh>& mesh = *(Ref<Mesh>*)inMesh;
        const auto& materials = mesh->GetMaterials();
        return materials.size();
    }

    void* NR_Texture2D_Constructor(uint32_t width, uint32_t height)
    {
        auto result = Texture2D::Create(ImageFormat::RGBA, width, height);
        return new Ref<Texture2D>(result);
    }

    void NR_Texture2D_Destructor(Ref<Texture2D>* _this)
    {
        delete _this;
    }

    void NR_Texture2D_SetData(Ref<Texture2D>* _this, MonoArray* inData, int32_t count)
    {
        Ref<Texture2D>& instance = *_this;

        uint32_t dataSize = count * sizeof(glm::vec4) / 4;

        instance->Lock();
        Buffer buffer = instance->GetWriteableBuffer();
        NR_CORE_ASSERT(dataSize <= buffer.Size);

        uint8_t* pixels = (uint8_t*)buffer.Data;
        uint32_t index = 0;
        for (int i = 0; i < instance->GetWidth() * instance->GetHeight(); ++i)
        {
            glm::vec4& value = mono_array_get(inData, glm::vec4, i);
            *pixels++ = (uint32_t)(value.x * 255.0f);
            *pixels++ = (uint32_t)(value.y * 255.0f);
            *pixels++ = (uint32_t)(value.z * 255.0f);
            *pixels++ = (uint32_t)(value.w * 255.0f);
        }

        instance->Unlock();
    }

    void NR_Material_Destructor(Ref<Material>* _this)
    {
        delete _this;
    }

    void NR_Material_SetFloat(Ref<Material>* _this, MonoString* uniform, float value)
    {
        Ref<Material>& instance = *(Ref<Material>*)_this;
        instance->Set(mono_string_to_utf8(uniform), value);
    }

    void NR_Material_SetTexture(Ref<Material>* _this, MonoString* uniform, Ref<Texture2D>* texture)
    {
        Ref<Material>& instance = *(Ref<Material>*)_this;
        instance->Set(mono_string_to_utf8(uniform), *texture);
    }

    void NR_Material_SetVector4(Ref<Material>* _this, MonoString* uniform, glm::vec4* value)
    {
        Ref<Material>& instance = *(Ref<Material>*)_this;
        instance->Set(mono_string_to_utf8(uniform), *value);
    }

    void NR_Material_Destructor(Ref<Material>* _this)
    {
        delete _this;
    }

    void NR_Material_SetFloat(Ref<Material>* _this, MonoString* uniform, float value)
    {
        Ref<Material>& instance = *(Ref<Material>*)_this;
        instance->Set(mono_string_to_utf8(uniform), value);
    }

    void NR_Material_SetVector3(Ref<Material>* _this, MonoString* uniform, glm::vec3* value)
    {
        Ref<Material>& instance = *(Ref<Material>*)_this;
        instance->Set(mono_string_to_utf8(uniform), *value);
    }

    void NR_Material_SetTexture(Ref<Material>* _this, MonoString* uniform, Ref<Texture2D>* texture)
    {
        Ref<Material>& instance = *(Ref<Material>*)_this;
        instance->Set(mono_string_to_utf8(uniform), *texture);
    }

    void* NR_MeshFactory_CreatePlane(float width, float height)
    {
        return new Ref<Mesh>(new Mesh("Assets/Models/Plane1m.obj"));
    }
}