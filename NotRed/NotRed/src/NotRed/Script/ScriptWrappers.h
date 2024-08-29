#pragma once

#include "NotRed/Script/ScriptEngine.h"
#include "NotRed/Core/KeyCodes.h"
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

    // Entity
    void NR_Entity_CreateComponent(uint64_t entityID, void* type);
    bool NR_Entity_HasComponent(uint64_t entityID, void* type);
    void NR_Entity_GetTransform(uint64_t entityID, glm::mat4* outTransform);
    void NR_Entity_SetTransform(uint64_t entityID, glm::mat4* inTransform);
    uint64_t NR_Entity_FindEntityByTag(MonoString* tag);
    
	void NR_TransformComponent_GetRelativeDirection(uint64_t entityID, glm::vec3* outDirection, glm::vec3* inAbsoluteDirection);

    void* NR_MeshComponent_GetMesh(uint64_t entityID);
    void NR_MeshComponent_SetMesh(uint64_t entityID, Ref<Mesh>* inMesh);

    void NR_RigidBody2DComponent_ApplyLinearImpulse(uint64_t entityID, glm::vec2* impulse, glm::vec2* offset, bool wake);
    void NR_RigidBody2DComponent_GetLinearVelocity(uint64_t entityID, glm::vec2* outVelocity);
    void NR_RigidBody2DComponent_SetLinearVelocity(uint64_t entityID, glm::vec2* velocity);

    void NR_RigidBodyComponent_AddForce(uint64_t entityID, glm::vec3* force, ForceMode foceMode);
    void NR_RigidBodyComponent_AddTorque(uint64_t entityID, glm::vec3* torque, ForceMode forceMode);
    void NR_RigidBodyComponent_GetLinearVelocity(uint64_t entityID, glm::vec3* outVelocity);
    void NR_RigidBodyComponent_SetLinearVelocity(uint64_t entityID, glm::vec3* velocity);

    // Renderer
    // Texture2D
    void* NR_Texture2D_Constructor(uint32_t width, uint32_t height);
    void NR_Texture2D_Destructor(Ref<Texture2D>* _this);
    void NR_Texture2D_SetData(Ref<Texture2D>* _this, MonoArray* inData, int32_t count);

    // Material
    void NR_Material_Destructor(Ref<Material>* _this);
    void NR_Material_SetFloat(Ref<Material>* _this, MonoString* uniform, float value);
    void NR_Material_SetTexture(Ref<Material>* _this, MonoString* uniform, Ref<Texture2D>* texture);

    void NR_MaterialInstance_Destructor(Ref<MaterialInstance>* _this);
    void NR_MaterialInstance_SetFloat(Ref<MaterialInstance>* _this, MonoString* uniform, float value);
    void NR_MaterialInstance_SetVector3(Ref<MaterialInstance>* _this, MonoString* uniform, glm::vec3* value);
    void NR_MaterialInstance_SetVector4(Ref<MaterialInstance>* _this, MonoString* uniform, glm::vec4* value);
    void NR_MaterialInstance_SetTexture(Ref<MaterialInstance>* _this, MonoString* uniform, Ref<Texture2D>* texture);

    // Mesh
    Ref<Mesh>* NR_Mesh_Constructor(MonoString* filepath);
    void NR_Mesh_Destructor(Ref<Mesh>* _this);
    Ref<Material>* NR_Mesh_GetMaterial(Ref<Mesh>* inMesh);
    Ref<MaterialInstance>* NR_Mesh_GetMaterialByIndex(Ref<Mesh>* inMesh, int index);
    int NR_Mesh_GetMaterialCount(Ref<Mesh>* inMesh);

    void* NR_MeshFactory_CreatePlane(float width, float height);
}