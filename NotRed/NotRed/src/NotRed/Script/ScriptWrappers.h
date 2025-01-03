#pragma once

#include <glm/glm.hpp>

#include "NotRed/Core/Input.h"
#include "NotRed/Physics/PhysicsManager.h"

#include "NotRed/Audio/Sound.h"

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

    // Scene
    MonoArray* NR_Scene_GetEntities();

    struct RaycastData
    {
        glm::vec3 Origin;
        glm::vec3 Direction;
        float MaxDistance;
        MonoArray* RequiredComponentTypes;
    };

    // Physics
    bool NR_Physics_Raycast(RaycastData* inData, RaycastHit* hit);
    MonoArray* NR_Physics_OverlapBox(glm::vec3* origin, glm::vec3* halfSize);
    MonoArray* NR_Physics_OverlapSphere(glm::vec3* origin, float radius);
    MonoArray* NR_Physics_OverlapCapsule(glm::vec3* origin, float radius, float halfHeight);
    int32_t NR_Physics_OverlapBoxNonAlloc(glm::vec3* origin, glm::vec3* halfSize, MonoArray* outColliders);
    int32_t NR_Physics_OverlapCapsuleNonAlloc(glm::vec3* origin, float radius, float halfHeight, MonoArray* outColliders);
    int32_t NR_Physics_OverlapSphereNonAlloc(glm::vec3* origin, float radius, MonoArray* outColliders);
    void NR_Physics_GetGravity(glm::vec3* outGravity);
    void NR_Physics_SetGravity(glm::vec3* inGravity);

    // Entity
    void NR_Entity_SetParent(uint64_t entityID, uint64_t parentID);
    uint64_t NR_Entity_GetParent(uint64_t entityID);
    MonoArray* NR_Entity_GetChildren(uint64_t entityID);
    uint64_t NR_Entity_CreateEntity(uint64_t entityID);
    uint64_t NR_Entity_Instantiate(uint64_t entityID, uint64_t prefabID);
    uint64_t NR_Entity_InstantiateWithTranslation(uint64_t entityID, uint64_t prefabID, glm::vec3* tranlation);
    void NR_Entity_DestroyEntity(uint64_t entityID);
    void NR_Entity_CreateComponent(uint64_t entityID, void* type);
    bool NR_Entity_HasComponent(uint64_t entityID, void* type);
    uint64_t NR_Entity_FindEntityByTag(MonoString* tag);
    uint64_t NR_Entity_InstantiateEntity();

    void NR_TransformComponent_GetTransform(uint64_t entityID, TransformComponent* outTransform);
    void NR_TransformComponent_SetTransform(uint64_t entityID, TransformComponent* inTransform);
    void NR_TransformComponent_GetTranslation(uint64_t entityID, glm::vec3* outTranslation);
    void NR_TransformComponent_SetTranslation(uint64_t entityID, glm::vec3* inTranslation);
    void NR_TransformComponent_GetRotation(uint64_t entityID, glm::vec3* outRotation);
    void NR_TransformComponent_SetRotation(uint64_t entityID, glm::vec3* inRotation);
    void NR_TransformComponent_GetScale(uint64_t entityID, glm::vec3* outScale);
    void NR_TransformComponent_SetScale(uint64_t entityID, glm::vec3* inScale);
    void NR_TransformComponent_GetWorldSpaceTransform(uint64_t entityID, TransformComponent* outTransform);

    MonoString* NR_TagComponent_GetTag(uint64_t entityID);
    void NR_TagComponent_SetTag(uint64_t entityID, MonoString* tag);

    void* NR_MeshComponent_GetMesh(uint64_t entityID);
    void NR_MeshComponent_SetMesh(uint64_t entityID, Ref<Mesh>* inMesh);
    bool NR_MeshComponent_HasMaterial(uint64_t entityID, int index);
    Ref<MaterialAsset>* NR_MeshComponent_GetMaterial(uint64_t entityID, int index);
    bool NR_MeshComponent_GetIsAnimated(uint64_t entityID);
    bool NR_MeshComponent_GetIsAnimationPlaying(uint64_t entityID);
    void NR_MeshComponent_SetIsAnimationPlaying(uint64_t entityID, bool value);

    void NR_RigidBody2DComponent_GetBodyType(uint64_t entityID, RigidBody2DComponent::Type* type);
    void NR_RigidBody2DComponent_SetBodyType(uint64_t entityID, RigidBody2DComponent::Type* type);
    void NR_RigidBody2DComponent_ApplyImpulse(uint64_t entityID, glm::vec2* impulse, glm::vec2* offset, bool wake);
    void NR_RigidBody2DComponent_GetVelocity(uint64_t entityID, glm::vec2* outVelocity);
    void NR_RigidBody2DComponent_SetVelocity(uint64_t entityID, glm::vec2* velocity);

    RigidBodyComponent::Type NR_RigidBodyComponent_GetBodyType(uint64_t entityID);
    void NR_RigidBodyComponent_SetBodyType(uint64_t entityID, RigidBodyComponent::Type type);
    void NR_RigidBodyComponent_GetRotation(uint64_t entityID, glm::vec3* rotation);
    void NR_RigidBodyComponent_SetRotation(uint64_t entityID, glm::vec3* rotation);
    void NR_RigidBodyComponent_Rotate(uint64_t entityID, glm::vec3* rotation);
    void NR_RigidBodyComponent_AddForce(uint64_t entityID, glm::vec3* force, ForceMode foceMode);	
    void NR_RigidBodyComponent_GetPosition(uint64_t entityID, glm::vec3* outTranslation);
    void NR_RigidBodyComponent_SetPosition(uint64_t entityID, glm::vec3* inTranslation);
    void NR_RigidBodyComponent_AddTorque(uint64_t entityID, glm::vec3* torque, ForceMode forceMode);
    void NR_RigidBodyComponent_GetVelocity(uint64_t entityID, glm::vec3* outVelocity);
    void NR_RigidBodyComponent_SetVelocity(uint64_t entityID, glm::vec3* velocity);
    void NR_RigidBodyComponent_GetAngularVelocity(uint64_t entityID, glm::vec3* outVelocity);
    void NR_RigidBodyComponent_SetAngularVelocity(uint64_t entityID, glm::vec3* velocity);
    float NR_RigidBodyComponent_GetMaxVelocity(uint64_t entityID);
    void NR_RigidBodyComponent_SetMaxVelocity(uint64_t entityID, float maxVelocity);
    float NR_RigidBodyComponent_GetMaxAngularVelocity(uint64_t entityID);
    void NR_RigidBodyComponent_SetMaxAngularVelocity(uint64_t entityID, float maxVelocity);
    uint32_t NR_RigidBodyComponent_GetLayer(uint64_t entityID);
    float NR_RigidBodyComponent_GetMass(uint64_t entityID);
    void NR_RigidBodyComponent_SetMass(uint64_t entityID, float mass);
    void NR_RigidBodyComponent_GetKinematicTarget(uint64_t entityID, glm::vec3* outTargetPosition, glm::vec3* outTargetRotation);
    void NR_RigidBodyComponent_SetKinematicTarget(uint64_t entityID, glm::vec3* inTargetPosition, glm::vec3* inTargetRotation);

    void NR_BoxColliderComponent_GetSize(uint64_t entityID, glm::vec3* outSize);
    void NR_BoxColliderComponent_SetSize(uint64_t entityID, glm::vec3* inSize);
    void NR_BoxColliderComponent_GetOffset(uint64_t entityID, glm::vec3* outOffset);
    void NR_BoxColliderComponent_SetOffset(uint64_t entityID, glm::vec3* inOffset);
    bool NR_BoxColliderComponent_IsTrigger(uint64_t entityID);
    void NR_BoxColliderComponent_SetTrigger(uint64_t entityID, bool isTrigger);

    float NR_SphereColliderComponent_GetRadius(uint64_t entityID);
    void NR_SphereColliderComponent_SetRadius(uint64_t entityID, float inRadius);
    void NR_SphereColliderComponent_GetOffset(uint64_t entityID, glm::vec3* outOffset);
    void NR_SphereColliderComponent_SetOffset(uint64_t entityID, glm::vec3* inOffset);
    bool NR_SphereColliderComponent_IsTrigger(uint64_t entityID);
    void NR_SphereColliderComponent_SetTrigger(uint64_t entityID, bool isTrigger);

    float NR_CapsuleColliderComponent_GetRadius(uint64_t entityID);
    void NR_CapsuleColliderComponent_SetRadius(uint64_t entityID, float inRadius);
    float NR_CapsuleColliderComponent_GetHeight(uint64_t entityID);
    void NR_CapsuleColliderComponent_SetHeight(uint64_t entityID, float inHeight);
    void NR_CapsuleColliderComponent_GetOffset(uint64_t entityID, glm::vec3* outOffset);
    void NR_CapsuleColliderComponent_SetOffset(uint64_t entityID, glm::vec3* inOffset);
    bool NR_CapsuleColliderComponent_IsTrigger(uint64_t entityID);
    void NR_CapsuleColliderComponent_SetTrigger(uint64_t entityID, bool isTrigger);

    Ref<Mesh>* NR_MeshColliderComponent_GetColliderMesh(uint64_t entityID);
    void NR_MeshColliderComponent_SetColliderMesh(uint64_t entityID, Ref<Mesh>* inMesh);
    bool NR_MeshColliderComponent_IsConvex(uint64_t entityID);
    void NR_MeshColliderComponent_SetConvex(uint64_t entityID, bool convex);
    bool NR_MeshColliderComponent_IsTrigger(uint64_t entityID);
    void NR_MeshColliderComponent_SetTrigger(uint64_t entityID, bool isTrigger);

    // Texture2D
    void* NR_Texture2D_Constructor(uint32_t width, uint32_t height);
    void NR_Texture2D_Destructor(Ref<Texture2D>* _this);
    void NR_Texture2D_SetData(Ref<Texture2D>* _this, MonoArray* inData, int32_t count);

    // Material
    void NR_Material_Destructor(Ref<MaterialAsset>* _this);
    void NR_Material_GetAlbedoColor(Ref<MaterialAsset>* _this, glm::vec3* outAlbedoColor);
    void NR_Material_SetAlbedoColor(Ref<MaterialAsset>* _this, glm::vec3* inAlbedoColor);

    void NR_Material_GetMetalness(Ref<MaterialAsset>* _this, float* outMetalness);
    void NR_Material_SetMetalness(Ref<MaterialAsset>* _this, float inMetalness);
    void NR_Material_GetRoughness(Ref<MaterialAsset>* _this, float* outRoughness);
    void NR_Material_SetRoughness(Ref<MaterialAsset>* _this, float inRoughness);
    void NR_Material_GetEmission(Ref<MaterialAsset>* _this, float* outEmission);
    void NR_Material_SetEmission(Ref<MaterialAsset>* _this, float inEmission);
    void NR_Material_SetFloat(Ref<MaterialAsset>* _this, MonoString* uniform, float value);
    void NR_Material_SetTexture(Ref<MaterialAsset>* _this, MonoString* uniform, Ref<Texture2D>* texture);

    // Mesh
    Ref<Mesh>* NR_Mesh_Constructor(MonoString* filepath);
    void NR_Mesh_Destructor(Ref<Mesh>* _this);
    Ref<MaterialAsset>* NR_Mesh_GetMaterial(Ref<Mesh>* inMesh);
    Ref<MaterialAsset>* NR_Mesh_GetMaterialByIndex(Ref<Mesh>* inMesh, int index);
    uint32_t NR_Mesh_GetMaterialCount(Ref<Mesh>* inMesh);

    void* NR_MeshFactory_CreatePlane(float width, float height);

    // Audio
    bool NR_AudioComponent_IsPlaying(uint64_t entityID);
    bool NR_AudioComponent_Play(uint64_t entityID, float startTime = 0.0f);
    bool NR_AudioComponent_Stop(uint64_t entityID);
    bool NR_AudioComponent_Pause(uint64_t entityID);

    void NR_AudioComponent_SetSound(uint64_t entityID, Ref<AudioFile>* sound);
    void NR_AudioComponent_SetSoundPath(uint64_t entityID, MonoString* filepath);
    void NR_AudioComponent_SetVolumeMult(uint64_t entityID, float volumeMult);
    void NR_AudioComponent_SetPitchMult(uint64_t entityID, float pitchMult);
    void NR_AudioComponent_SetLooping(uint64_t entityID, bool looping);
    float NR_AudioComponent_GetMasterReverbSend(uint64_t entityID);
    void NR_AudioComponent_SetMasterReverbSend(uint64_t entityID, float sendLevel);

    MonoString* NR_AudioComponent_GetSound(uint64_t entityID);
    float NR_AudioComponent_GetVolumeMult(uint64_t entityID);
    float NR_AudioComponent_GetPitchMult(uint64_t entityID);
    bool NR_AudioComponent_GetLooping(uint64_t entityID);

    bool NR_Audio_PlaySound2DAsset(Ref<AudioFile>* sound, float volume = 1.0f, float pitch = 1.0f);
    bool NR_Audio_PlaySound2DAssetPath(MonoString* filepath, float volume = 1.0f, float pitch = 1.0f);
    bool NR_Audio_PlaySoundAtLocationAsset(Ref<AudioFile>* sound, glm::vec3* location, float volume = 1.0f, float pitch = 1.0f);
    bool NR_Audio_PlaySoundAtLocationAssetPath(MonoString* filepath, glm::vec3* location, float volume = 1.0f, float pitch = 1.0f);

    Ref<AudioFile>* NR_SimpleSound_Constructor(MonoString* filepath);

    void NR_SimpleSound_Destructor(Ref<AudioFile>* _this);

    uint64_t NR_AudioCreateSound2DAsset(Ref<AudioFile>* sound, float volume = 1.0f, float pitch = 1.0f);
    uint64_t NR_AudioCreateSound2DPath(MonoString* filepath, float volume = 1.0f, float pitch = 1.0f);
    uint64_t NR_AudioCreateSound3DAsset(Ref<AudioFile>* sound, glm::vec3* location, float volume = 1.0f, float pitch = 1.0f);
    uint64_t NR_AudioCreateSound3DPath(MonoString* filepath, glm::vec3* location, float volume = 1.0f, float pitch = 1.0f);

    enum class LogLevel : int32_t
    {
        Trace       = 1 << 0,
        Debug       = 1 << 1,
        Info        = 1 << 2,
        Warn        = 1 << 3,
        Error       = 1 << 4,
        Critical    = 1 << 5
    };
    void NR_Log_LogMessage(LogLevel level, MonoString* message);
}