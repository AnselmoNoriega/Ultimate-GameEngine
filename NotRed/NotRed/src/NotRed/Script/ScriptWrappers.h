#pragma once

#include "NotRed/Script/ScriptEngine.h"
#include "NotRed/Core/KeyCodes.h"

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
    void NR_Entity_CreateComponent(uint32_t sceneID, uint32_t entityID, void* type);
    bool NR_Entity_HasComponent(uint32_t sceneID, uint32_t entityID, void* type);
    void NR_Entity_GetTransform(uint32_t sceneID, uint32_t entityID, glm::mat4* outTransform);
    void NR_Entity_SetTransform(uint32_t sceneID, uint32_t entityID, glm::mat4* inTransform);

    void* NR_MeshComponent_GetMesh(uint32_t sceneID, uint32_t entityID);
    void NR_MeshComponent_SetMesh(uint32_t sceneID, uint32_t entityID, Ref<Mesh>* inMesh);

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
    void NR_MaterialInstance_SetTexture(Ref<MaterialInstance>* _this, MonoString* uniform, Ref<Texture2D>* texture);

    // Mesh
    Ref<Mesh>* NR_Mesh_Constructor(MonoString* filepath);
    void NR_Mesh_Destructor(Ref<Mesh>* _this);
    Ref<Material>* NR_Mesh_GetMaterial(Ref<Mesh>* inMesh);
    Ref<MaterialInstance>* NR_Mesh_GetMaterialByIndex(Ref<Mesh>* inMesh, int index);
    int NR_Mesh_GetMaterialCount(Ref<Mesh>* inMesh);

    void* NR_MeshFactory_CreatePlane(float width, float height);
}