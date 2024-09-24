#pragma once

#include "NotRed/Script/ScriptEngine.h"
#include "NotRed/Core/Input.h"
#include "NotRed/Physics/PhysicsManager.h"

#include <glm/glm.hpp>

extern "C" {
    typedef struct _MonoString MonoString;
    typedef struct _MonoArray MonoArray;
}

namespace NR::Script
{
    // Math
    float NR_Noise_PerlinNoise(float x, float y);

    // Input
    bool NR_Input_IsKeyPressed(KeyCode key);
    bool NR_Input_IsMouseButtonPressed(MouseButton button);
    void NR_Input_GetMousePosition(glm::vec2* outPosition);
    void NR_Input_SetCursorMode(CursorMode mode);
    CursorMode NR_Input_GetCursorMode();

    // Physics
    MonoArray* NR_Physics_OverlapBox(glm::vec3* origin, glm::vec3* halfSize);
    MonoArray* NR_Physics_OverlapSphere(glm::vec3* origin, float radius);
    MonoArray* NR_Physics_OverlapCapsule(glm::vec3* origin, float radius, float halfHeight);
    int32_t NR_Physics_OverlapBoxNonAlloc(glm::vec3* origin, glm::vec3* halfSize, MonoArray* outColliders);
    int32_t NR_Physics_OverlapCapsuleNonAlloc(glm::vec3* origin, float radius, float halfHeight, MonoArray* outColliders);
    int32_t NR_Physics_OverlapSphereNonAlloc(glm::vec3* origin, float radius, MonoArray* outColliders);

    // Entity
    void NR_Entity_CreateComponent(uint64_t entityID, void* type);
    bool NR_Entity_HasComponent(uint64_t entityID, void* type);
    uint64_t NR_Entity_FindEntityByTag(MonoString* tag);

    void NR_TransformComponent_GetTransform(uint64_t entityID, TransformComponent* outTransform);
    void NR_TransformComponent_SetTransform(uint64_t entityID, TransformComponent* inTransform);
    void NR_TransformComponent_GetTranslation(uint64_t entityID, glm::vec3* outTranslation);
    void NR_TransformComponent_SetTranslation(uint64_t entityID, glm::vec3* inTranslation);
    void NR_TransformComponent_GetRotation(uint64_t entityID, glm::vec3* outRotation);
    void NR_TransformComponent_SetRotation(uint64_t entityID, glm::vec3* inRotation);
    void NR_TransformComponent_GetScale(uint64_t entityID, glm::vec3* outScale);
    void NR_TransformComponent_SetScale(uint64_t entityID, glm::vec3* inScale);

    void* NR_MeshComponent_GetMesh(uint64_t entityID);
    void NR_MeshComponent_SetMesh(uint64_t entityID, Ref<Mesh>* inMesh);

    void NR_RigidBody2DComponent_ApplyImpulse(uint64_t entityID, glm::vec2* impulse, glm::vec2* offset, bool wake);
    void NR_RigidBody2DComponent_GetVelocity(uint64_t entityID, glm::vec2* outVelocity);
    void NR_RigidBody2DComponent_SetVelocity(uint64_t entityID, glm::vec2* velocity);

    RigidBodyComponent::Type NR_RigidBodyComponent_GetBodyType(uint64_t entityID);
    void NR_RigidBodyComponent_AddForce(uint64_t entityID, glm::vec3* force, ForceMode foceMode);
    void NR_RigidBodyComponent_AddTorque(uint64_t entityID, glm::vec3* torque, ForceMode forceMode);
    void NR_RigidBodyComponent_GetVelocity(uint64_t entityID, glm::vec3* outVelocity);
    void NR_RigidBodyComponent_SetVelocity(uint64_t entityID, glm::vec3* velocity);
    void NR_RigidBodyComponent_GetAngularVelocity(uint64_t entityID, glm::vec3* outVelocity);
    void NR_RigidBodyComponent_SetAngularVelocity(uint64_t entityID, glm::vec3* velocity);
    float NR_RigidBodyComponent_GetMaxVelocity(uint64_t entityID);
    void NR_RigidBodyComponent_SetMaxVelocity(uint64_t entityID, float maxVelocity);
    float NR_RigidBodyComponent_GetMaxAngularVelocity(uint64_t entityID);
    void NR_RigidBodyComponent_SetMaxAngularVelocity(uint64_t entityID, float maxVelocity);
    void NR_RigidBodyComponent_Rotate(uint64_t entityID, glm::vec3* rotation);
    uint32_t NR_RigidBodyComponent_GetLayer(uint64_t entityID);
    float NR_RigidBodyComponent_GetMass(uint64_t entityID);
    void NR_RigidBodyComponent_SetMass(uint64_t entityID, float mass);

    // Renderer
    // Texture2D
    void* NR_Texture2D_Constructor(uint32_t width, uint32_t height);
    void NR_Texture2D_Destructor(Ref<Texture2D>* _this);
    void NR_Texture2D_SetData(Ref<Texture2D>* _this, MonoArray* inData, int32_t count);

    // Material
    void NR_Material_Destructor(Ref<Material>* _this);
    void NR_Material_SetFloat(Ref<Material>* _this, MonoString* uniform, float value);
    void NR_Material_SetTexture(Ref<Material>* _this, MonoString* uniform, Ref<Texture2D>* texture);

    void NR_MaterialInstance_Destructor(Ref<Material>* _this);
    void NR_MaterialInstance_SetFloat(Ref<Material>* _this, MonoString* uniform, float value);
    void NR_MaterialInstance_SetVector3(Ref<Material>* _this, MonoString* uniform, glm::vec3* value);
    void NR_MaterialInstance_SetVector4(Ref<Material>* _this, MonoString* uniform, glm::vec4* value);
    void NR_MaterialInstance_SetTexture(Ref<Material>* _this, MonoString* uniform, Ref<Texture2D>* texture);

    // Mesh
    Ref<Mesh>* NR_Mesh_Constructor(MonoString* filepath);
    void NR_Mesh_Destructor(Ref<Mesh>* _this);
    Ref<Material>* NR_Mesh_GetMaterial(Ref<Mesh>* inMesh);
    Ref<Material>* NR_Mesh_GetMaterialByIndex(Ref<Mesh>* inMesh, int index);
    int NR_Mesh_GetMaterialCount(Ref<Mesh>* inMesh);

    void* NR_MeshFactory_CreatePlane(float width, float height);
}