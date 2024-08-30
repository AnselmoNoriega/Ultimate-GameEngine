#include "nrpch.h"
#include "ScriptWrappers.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <imgui.h>

#include <glm/gtc/type_ptr.hpp>
#include <mono/jit/jit.h>

#include <box2d/box2d.h>

#include <PxPhysicsAPI.h>
#include "NotRed/Physics/PhysicsUtil.h"
#include "NotRed/Physics/PhysicsWrappers.h"

#include "NotRed/Core/Math/Noise.h"

#include "NotRed/Scene/Scene.h"
#include "NotRed/Scene/Entity.h"
#include "NotRed/Scene/Components.h"

namespace NR
{
    extern std::unordered_map<MonoType*, std::function<bool(Entity&)>> sHasComponentFuncs;
    extern std::unordered_map<MonoType*, std::function<void(Entity&)>> sCreateComponentFuncs;
}

namespace NR::Script
{
    enum class ComponentID
    {
        None,
        Transform,
        Mesh,
        Script,
        SpriteRenderer
    };

    static std::tuple<glm::vec3, glm::quat, glm::vec3> DecomposeTransform(const glm::mat4& transform)
    {
        glm::vec3 scale, translation, skew;
        glm::vec4 perspective;
        glm::quat orientation;
        glm::decompose(transform, scale, orientation, translation, skew, perspective);

        return { translation, orientation, scale };
    }

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

    void NR_Entity_GetTransform(uint64_t entityID, glm::mat4* outTransform)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        auto& transformComponent = entity.GetComponent<TransformComponent>();
        memcpy(outTransform, glm::value_ptr(transformComponent.Transform), sizeof(glm::mat4));
    }

    void NR_Entity_SetTransform(uint64_t entityID, glm::mat4* inTransform)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        auto& transformComponent = entity.GetComponent<TransformComponent>();
        memcpy(glm::value_ptr(transformComponent.Transform), inTransform, sizeof(glm::mat4));
    }

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

    void NR_TransformComponent_GetRelativeDirection(uint64_t entityID, glm::vec3* outDirection, glm::vec3* inAbsoluteDirection)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        auto& transformComponent = entity.GetComponent<TransformComponent>();

        auto [position, rotation, scale] = DecomposeTransform(transformComponent.Transform);
        *outDirection = glm::rotate(rotation, *inAbsoluteDirection);
    }

    void NR_TransformComponent_GetRotation(uint64_t entityID, glm::vec3* outRotation)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        auto& transformComponent = entity.GetComponent<TransformComponent>();
        auto [position, rotationQuat, scale] = DecomposeTransform(transformComponent.Transform);
        *outRotation = glm::degrees(glm::eulerAngles(rotationQuat));
    }

    void NR_TransformComponent_SetRotation(uint64_t entityID, glm::vec3* inRotation)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");
        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        glm::mat4& transform = entity.Transform();

        auto [translation, rotationQuat, scale] = DecomposeTransform(transform);
        transform = glm::translate(glm::mat4(1.0f), translation) *
            glm::toMat4(glm::quat(glm::radians(*inRotation))) *
            glm::scale(glm::mat4(1.0f), scale);
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

    static void AddCollidersToArray(MonoArray* array, const std::vector<physx::PxOverlapHit>& hits)
    {
        uint32_t arrayIndex = 0;
        for (const auto& hit : hits)
        {
            Entity& entity = *(Entity*)hit.actor->userData;

            if (entity.HasComponent<BoxColliderComponent>())
            {
                auto& boxCollider = entity.GetComponent<BoxColliderComponent>();

                void* data[] = {
                    &entity.GetID(),
                    &boxCollider.Size,
                    &boxCollider.Offset,
                    &boxCollider.IsTrigger
                };

                MonoObject* obj = ScriptEngine::Construct("NR.BoxCollider:.ctor(ulong,bool,Vector3,Vector3)", true, data);
                mono_array_set(array, MonoObject*, arrayIndex++, obj);
            }

            if (entity.HasComponent<SphereColliderComponent>())
            {
                auto& sphereCollider = entity.GetComponent<SphereColliderComponent>();

                void* data[] = {
                    &entity.GetID(),
                    &sphereCollider.Radius,
                    &sphereCollider.IsTrigger
                };

                MonoObject* obj = ScriptEngine::Construct("NR.SphereCollider:.ctor(ulong,bool,float)", true, data);
                mono_array_set(array, MonoObject*, arrayIndex++, obj);
            }

            if (entity.HasComponent<CapsuleColliderComponent>())
            {
                auto& capsuleCollider = entity.GetComponent<CapsuleColliderComponent>();

                void* data[] = {
                    &entity.GetID(),
                    &capsuleCollider.Radius,
                    &capsuleCollider.Height,
                    &capsuleCollider.IsTrigger
                };

                MonoObject* obj = ScriptEngine::Construct("NR.CapsuleCollider:.ctor(ulong,bool,float,float)", true, data);
                mono_array_set(array, MonoObject*, arrayIndex++, obj);
            }

            if (entity.HasComponent<MeshColliderComponent>())
            {
                auto& meshCollider = entity.GetComponent<MeshColliderComponent>();

                Ref<Mesh>* mesh = new Ref<Mesh>(meshCollider.CollisionMesh);
                void* data[] = {
                    &entity.GetID(),
                    &mesh,
                    &meshCollider.IsTrigger
                };

                MonoObject* obj = ScriptEngine::Construct("NR.MeshCollider:.ctor(ulong,bool,intptr)", true, data);
                mono_array_set(array, MonoObject*, arrayIndex++, obj);
            }
        }

    }

    MonoArray* NR_Physics_OverlapBox(glm::vec3* origin, glm::vec3* halfSize)
    {
        MonoArray* outColliders = nullptr;
        std::vector<physx::PxOverlapHit> buffer;

        if (PhysicsWrappers::OverlapBox(*origin, *halfSize, buffer))
        {
            outColliders = mono_array_new(mono_domain_get(), ScriptEngine::GetCoreClass("NR.Collider"), buffer.size());
            AddCollidersToArray(outColliders, buffer);
        }
        return outColliders;
    }

    MonoArray* NR_Physics_OverlapSphere(glm::vec3* origin, float radius)
    {
        MonoArray* outColliders = nullptr;
        std::vector<physx::PxOverlapHit> buffer;
        if (PhysicsWrappers::OverlapSphere(*origin, radius, buffer))
        {
            outColliders = mono_array_new(mono_domain_get(), ScriptEngine::GetCoreClass("NR.Collider"), buffer.size());
            AddCollidersToArray(outColliders, buffer);
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

        physx::PxRigidActor* actor = (physx::PxRigidActor*)component.RuntimeActor;
        physx::PxRigidDynamic* dynamicActor = actor->is<physx::PxRigidDynamic>();
        NR_CORE_ASSERT(dynamicActor);

        NR_CORE_ASSERT(force);
        dynamicActor->addForce({ force->x, force->y, force->z }, (physx::PxForceMode::Enum)forceMode);
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

        physx::PxRigidActor* actor = (physx::PxRigidActor*)component.RuntimeActor;
        physx::PxRigidDynamic* dynamicActor = actor->is<physx::PxRigidDynamic>();
        NR_CORE_ASSERT(dynamicActor && torque);

        dynamicActor->addTorque({ torque->x, torque->y, torque->z }, (physx::PxForceMode::Enum)forceMode);
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

        physx::PxRigidActor* actor = (physx::PxRigidActor*)component.RuntimeActor;
        physx::PxRigidDynamic* dynamicActor = actor->is<physx::PxRigidDynamic>();
        NR_CORE_ASSERT(dynamicActor);

        NR_CORE_ASSERT(outVelocity);
        physx::PxVec3 velocity = dynamicActor->getLinearVelocity();
        *outVelocity = { velocity.x, velocity.y, velocity.z };
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

        physx::PxRigidActor* actor = (physx::PxRigidActor*)component.RuntimeActor;
        physx::PxRigidDynamic* dynamicActor = actor->is<physx::PxRigidDynamic>();
        NR_CORE_ASSERT(dynamicActor);

        NR_CORE_ASSERT(velocity);
        dynamicActor->setLinearVelocity({ velocity->x, velocity->y, velocity->z });
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

        physx::PxRigidActor* actor = (physx::PxRigidActor*)component.RuntimeActor;
        physx::PxRigidDynamic* dynamicActor = actor->is<physx::PxRigidDynamic>();
        NR_CORE_ASSERT(dynamicActor);

        physx::PxTransform transform = dynamicActor->getGlobalPose();
        transform.q *= (physx::PxQuat(glm::radians(rotation->x), { 1.0f, 0.0f, 0.0f })
            * physx::PxQuat(glm::radians(rotation->y), { 0.0f, 1.0f, 0.0f })
            * physx::PxQuat(glm::radians(rotation->z), { 0.0f, 0.0f, 1.0f }));
        dynamicActor->setGlobalPose(transform);
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

        physx::PxRigidActor* actor = (physx::PxRigidActor*)component.RuntimeActor;
        physx::PxRigidDynamic* dynamicActor = actor->is<physx::PxRigidDynamic>();
        NR_CORE_ASSERT(dynamicActor);

        return dynamicActor->getMass();
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

        physx::PxRigidActor* actor = (physx::PxRigidActor*)component.RuntimeActor;
        physx::PxRigidDynamic* dynamicActor = actor->is<physx::PxRigidDynamic>();
        NR_CORE_ASSERT(dynamicActor);

        component.Mass = mass;
        physx::PxRigidBodyExt::updateMassAndInertia(*dynamicActor, mass);
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
        return new Ref<Material>(mesh->GetMaterial());
    }

    Ref<MaterialInstance>* NR_Mesh_GetMaterialByIndex(Ref<Mesh>* inMesh, int index)
    {
        Ref<Mesh>& mesh = *(Ref<Mesh>*)inMesh;
        const auto& materials = mesh->GetMaterials();

        NR_CORE_ASSERT(index < materials.size());
        return new Ref<MaterialInstance>(materials[index]);
    }

    int NR_Mesh_GetMaterialCount(Ref<Mesh>* inMesh)
    {
        Ref<Mesh>& mesh = *(Ref<Mesh>*)inMesh;
        const auto& materials = mesh->GetMaterials();
        return materials.size();
    }

    void* NR_Texture2D_Constructor(uint32_t width, uint32_t height)
    {
        auto result = Texture2D::Create(TextureFormat::RGBA, width, height);
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

    void NR_MaterialInstance_SetVector4(Ref<MaterialInstance>* _this, MonoString* uniform, glm::vec4* value)
    {
        Ref<MaterialInstance>& instance = *(Ref<MaterialInstance>*)_this;
        instance->Set(mono_string_to_utf8(uniform), *value);
    }

    void NR_MaterialInstance_Destructor(Ref<MaterialInstance>* _this)
    {
        delete _this;
    }

    void NR_MaterialInstance_SetFloat(Ref<MaterialInstance>* _this, MonoString* uniform, float value)
    {
        Ref<MaterialInstance>& instance = *(Ref<MaterialInstance>*)_this;
        instance->Set(mono_string_to_utf8(uniform), value);
    }

    void NR_MaterialInstance_SetVector3(Ref<MaterialInstance>* _this, MonoString* uniform, glm::vec3* value)
    {
        Ref<MaterialInstance>& instance = *(Ref<MaterialInstance>*)_this;
        instance->Set(mono_string_to_utf8(uniform), *value);
    }

    void NR_MaterialInstance_SetTexture(Ref<MaterialInstance>* _this, MonoString* uniform, Ref<Texture2D>* texture)
    {
        Ref<MaterialInstance>& instance = *(Ref<MaterialInstance>*)_this;
        instance->Set(mono_string_to_utf8(uniform), *texture);
    }

    void* NR_MeshFactory_CreatePlane(float width, float height)
    {
        return new Ref<Mesh>(new Mesh("Assets/Models/Plane1m.obj"));
    }
}