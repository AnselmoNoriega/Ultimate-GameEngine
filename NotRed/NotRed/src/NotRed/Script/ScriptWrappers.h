#pragma once

#include "NotRed/Core/Input.h"
#include "NotRed/Physics/3D/PhysicsManager.h"
#include "NotRed/Audio/Sound.h"

#include <glm/glm.hpp>

extern "C" {
	typedef struct _MonoObject MonoObject;
	typedef struct _MonoString MonoString;
	typedef struct _MonoArray MonoArray;
}

namespace NR::Audio
{
	class CommandID;
	struct Transform;
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
		bool NR_Input_IsControllerPresent(int id);
		MonoArray* NR_Input_GetConnectedControllerIDs();
		MonoString* NR_Input_GetControllerName(int id);
		bool NR_Input_IsControllerButtonPressed(int id, int button);
		float NR_Input_GetControllerAxis(int id, int axis);
		uint8_t NR_Input_GetControllerHat(int id, int hat);

		// Scene
		MonoArray* NR_Scene_GetEntities();

		// Physics
		struct RaycastData
		{
			glm::vec3 Origin;
			glm::vec3 Direction;
			float MaxDistance;
			MonoArray* RequiredComponentTypes;
		};

		struct RaycastData2D
		{
			glm::vec2 Origin;
			glm::vec2 Direction;
			float MaxDistance;
			MonoArray* RequiredComponentTypes;
		};

		bool NR_Physics_Raycast(RaycastData* inData, RaycastHit* hit);
		MonoArray* NR_Physics_OverlapBox(glm::vec3* origin, glm::vec3* halfSize);
		MonoArray* NR_Physics_OverlapCapsule(glm::vec3* origin, float radius, float halfHeight);
		MonoArray* NR_Physics_OverlapSphere(glm::vec3* origin, float radius);
		int32_t NR_Physics_OverlapBoxNonAlloc(glm::vec3* origin, glm::vec3* halfSize, MonoArray* outColliders);
		int32_t NR_Physics_OverlapCapsuleNonAlloc(glm::vec3* origin, float radius, float halfHeight, MonoArray* outColliders);
		int32_t NR_Physics_OverlapSphereNonAlloc(glm::vec3* origin, float radius, MonoArray* outColliders);
		void NR_Physics_GetGravity(glm::vec3* outGravity);
		void NR_Physics_SetGravity(glm::vec3* inGravity);
		void NR_Physics_AddRadialImpulse(glm::vec3* inOrigin, float radius, float strength, EFalloffMode falloff, bool velocityChange);

		// 2D Physics
		MonoArray* NR_Physics_Raycast2D(RaycastData2D* inData);

		// Entity
		uint64_t NR_Entity_GetParent(uint64_t entityID);
		void NR_Entity_SetParent(uint64_t entityID, uint64_t parentID);
		MonoArray* NR_Entity_GetChildren(uint64_t entityID);
		uint64_t NR_Entity_CreateEntity(uint64_t entityID);
		uint64_t NR_Entity_Instantiate(uint64_t entityID, uint64_t prefabID);
		uint64_t NR_Entity_InstantiateWithPosition(uint64_t entityID, uint64_t prefabID, glm::vec3* inPosition);
		uint64_t NR_Entity_InstantiateWithTransform(uint64_t entityID, uint64_t prefabID, glm::vec3* inPosition, glm::vec3* inRotation, glm::vec3* inScale);
		void NR_Entity_DestroyEntity(uint64_t entityID);
		void NR_Entity_CreateComponent(uint64_t entityID, void* type);
		bool NR_Entity_HasComponent(uint64_t entityID, void* type);
		uint64_t NR_Entity_FindEntityByTag(MonoString* tag);

		MonoString* NR_TagComponent_GetTag(uint64_t entityID);
		void NR_TagComponent_SetTag(uint64_t entityID, MonoString* tag);

		void NR_TransformComponent_GetTransform(uint64_t entityID, TransformComponent* outTransform);
		void NR_TransformComponent_SetTransform(uint64_t entityID, TransformComponent* inTransform);
		void NR_TransformComponent_GetPosition(uint64_t entityID, glm::vec3* outPosition);
		void NR_TransformComponent_SetPosition(uint64_t entityID, glm::vec3* inPosition);
		void NR_TransformComponent_GetRotation(uint64_t entityID, glm::vec3* outRotation);
		void NR_TransformComponent_SetRotation(uint64_t entityID, glm::vec3* inRotation);
		void NR_TransformComponent_GetScale(uint64_t entityID, glm::vec3* outScale);
		void NR_TransformComponent_SetScale(uint64_t entityID, glm::vec3* inScale);
		void NR_TransformComponent_GetWorldSpaceTransform(uint64_t entityID, TransformComponent* outTransform);
		void NR_Transform_TransformMultiply(const TransformComponent* a, const TransformComponent* b, TransformComponent* outTransform);

		void* NR_MeshComponent_GetMesh(uint64_t entityID, bool* outIsStatic);
		void NR_MeshComponent_SetMesh(uint64_t entityID, void* in, bool inIsStatic);
		bool NR_MeshComponent_HasMaterial(uint64_t entityID, int index);
		Ref<MaterialAsset>* NR_MeshComponent_GetMaterial(uint64_t entityID, int index);
		bool NR_MeshComponent_GetIsRigged(uint64_t entityID);

		bool NR_AnimationComponent_GetIsAnimationPlaying(uint64_t entityID);
		void NR_AnimationComponent_SetIsAnimationPlaying(uint64_t entityID, bool value);
		uint32_t NR_AnimationComponent_GetStateIndex(uint64_t entityID);
		void NR_AnimationComponent_SetStateIndex(uint64_t entityID, uint32_t value);
		bool NR_AnimationComponent_GetEnableRootMotion(uint64_t entityID);
		void NR_AnimationComponent_SetEnableRootMotion(uint64_t entityID, bool value);
		void NR_AnimationComponent_GetRootMotion(uint64_t entityID, TransformComponent* outTransform);

		MonoObject* NR_ScriptComponent_GetInstance(uint64_t entityID);

		void NR_PointLightComponent_GetRadiance(uint64_t entityID, glm::vec3* outRadiance);
		void NR_PointLightComponent_SetRadiance(uint64_t entityID, glm::vec3* inRadiance);

		void NR_RigidBody2DComponent_GetBodyType(uint64_t entityID, RigidBody2DComponent::Type* type);
		void NR_RigidBody2DComponent_SetBodyType(uint64_t entityID, RigidBody2DComponent::Type* type);
		void NR_RigidBody2DComponent_GetPosition(uint64_t entityID, glm::vec2* outPosition);
		void NR_RigidBody2DComponent_SetPosition(uint64_t entityID, glm::vec2* inPosition);
		void NR_RigidBody2DComponent_GetRotation(uint64_t entityID, float* outRotation);
		void NR_RigidBody2DComponent_SetRotation(uint64_t entityID, float* inRotation);
		void NR_RigidBody2DComponent_AddForce(uint64_t entityID, glm::vec2* force, glm::vec2* offset, bool wake);
		void NR_RigidBody2DComponent_AddTorque(uint64_t entityID, float* torque, bool wake);
		void NR_RigidBody2DComponent_ApplyLinearImpulse(uint64_t entityID, glm::vec2* impulse, glm::vec2* offset, bool wake);
		void NR_RigidBody2DComponent_ApplyAngularImpulse(uint64_t entityID, float* impulse, bool wake);
		void NR_RigidBody2DComponent_GetVelocity(uint64_t entityID, glm::vec2* outVelocity);
		void NR_RigidBody2DComponent_SetVelocity(uint64_t entityID, glm::vec2* inVelocity);
		void NR_RigidBody2DComponent_GetGravityScale(uint64_t entityID, float* outGravityScale);
		void NR_RigidBody2DComponent_SetGravityScale(uint64_t entityID, float* inGravityScale);
		void NR_RigidBody2DComponent_GetMass(uint64_t entityID, float* outMass);
		void NR_RigidBody2DComponent_SetMass(uint64_t entityID, float* inMass);

		RigidBodyComponent::Type NR_RigidBodyComponent_GetBodyType(uint64_t entityID);
		void NR_RigidBodyComponent_SetBodyType(uint64_t entityID, RigidBodyComponent::Type type);
		void NR_RigidBodyComponent_GetPosition(uint64_t entityID, glm::vec3* outPosition);
		void NR_RigidBodyComponent_SetPosition(uint64_t entityID, glm::vec3* inPosition);
		void NR_RigidBodyComponent_GetRotation(uint64_t entityID, glm::vec3* outRotation);
		void NR_RigidBodyComponent_SetRotation(uint64_t entityID, glm::vec3* inRotation);
		void NR_RigidBodyComponent_Rotate(uint64_t entityID, glm::vec3* inRotation);
		void NR_RigidBodyComponent_AddForce(uint64_t entityID, glm::vec3* force, ForceMode foceMode);
		void NR_RigidBodyComponent_AddTorque(uint64_t entityID, glm::vec3* torque, ForceMode forceMode);
		void NR_RigidBodyComponent_GetVelocity(uint64_t entityID, glm::vec3* outVelocity);
		void NR_RigidBodyComponent_SetVelocity(uint64_t entityID, glm::vec3* inVelocity);
		void NR_RigidBodyComponent_GetAngularVelocity(uint64_t entityID, glm::vec3* outVelocity);
		void NR_RigidBodyComponent_SetAngularVelocity(uint64_t entityID, glm::vec3* inVelocity);
		float NR_RigidBodyComponent_GetMaxVelocity(uint64_t entityID);
		void NR_RigidBodyComponent_SetMaxVelocity(uint64_t entityID, float maxVelocity);
		float NR_RigidBodyComponent_GetMaxAngularVelocity(uint64_t entityID);
		void NR_RigidBodyComponent_SetMaxAngularVelocity(uint64_t entityID, float maxVelocity);
		float NR_RigidBodyComponent_GetLinearDrag(uint64_t entityID);
		void NR_RigidBodyComponent_SetLinearDrag(uint64_t entityID, float linearDrag);
		float NR_RigidBodyComponent_GetAngularDrag(uint64_t entityID);
		void NR_RigidBodyComponent_SetAngularDrag(uint64_t entityID, float angularDrag);
		uint32_t NR_RigidBodyComponent_GetLayer(uint64_t entityID);
		float NR_RigidBodyComponent_GetMass(uint64_t entityID);
		void NR_RigidBodyComponent_SetMass(uint64_t entityID, float mass);
		void NR_RigidBodyComponent_GetKinematicTarget(uint64_t entityID, glm::vec3* outTargetPosition, glm::vec3* outTargetRotation);
		void NR_RigidBodyComponent_SetKinematicTarget(uint64_t entityID, glm::vec3* inTargetPosition, glm::vec3* inTargetRotation);
		void NR_RigidBodyComponent_ModifyLockFlag(uint64_t entityID, ActorLockFlag flag, bool value);
		bool NR_RigidBodyComponent_IsLockFlagSet(uint64_t entityID, ActorLockFlag flag);
		uint32_t NR_RigidBodyComponent_GetLockFlags(uint64_t entityID);
		bool NR_RigidBodyComponent_IsKinematic(uint64_t entityID);
		void NR_RigidBodyComponent_SetIsKinematic(uint64_t entityID, bool isKinematic);
		bool NR_RigidBodyComponent_IsSleeping(uint64_t entityID);
		void NR_RigidBodyComponent_SetIsSleeping(uint64_t entityID, bool isSleeping);

		float NR_CharacterControllerComponent_GetSlopeLimit(uint64_t entityID);
		void NR_CharacterControllerComponent_SetSlopeLimit(uint64_t entityID, float slopeLimitDeg);
		float NR_CharacterControllerComponent_GetStepOffset(uint64_t entityID);
		void NR_CharacterControllerComponent_SetStepOffset(uint64_t entityID, float stepOffset);
		void NR_CharacterControllerComponent_Move(uint64_t entityID, glm::vec3* displacement);
		void NR_CharacterControllerComponent_Jump(uint64_t entityID, float jumpPower);
		float NR_CharacterControllerComponent_GetSpeedDown(uint64_t entityID);
		bool NR_CharacterControllerComponent_IsGrounded(uint64_t entityID);
		CollisionFlags NR_CharacterControllerComponent_GetCollisionFlags(uint64_t entityID);

		uint64_t NR_FixedJointComponent_GetConnectedEntity(uint64_t entityID);
		void NR_FixedJointComponent_SetConnectedEntity(uint64_t entityID, uint64_t connectedEntity);
		bool NR_FixedJointComponent_IsBreakable(uint64_t entityID);
		void NR_FixedJointComponent_SetIsBreakable(uint64_t entityID, bool isBreakable);
		bool NR_FixedJointComponent_IsBroken(uint64_t entityID);
		void NR_FixedJointComponent_Break(uint64_t entityID);
		float NR_FixedJointComponent_GetBreakForce(uint64_t entityID);
		void NR_FixedJointComponent_SetBreakForce(uint64_t entityID, float breakForce);
		float NR_FixedJointComponent_GetBreakTorque(uint64_t entityID);
		void NR_FixedJointComponent_SetBreakTorque(uint64_t entityID, float breakTorque);
		bool NR_FixedJointComponent_IsCollisionEnabled(uint64_t entityID);
		void NR_FixedJointComponent_SetCollisionEnabled(uint64_t entityID, bool isCollisionEnabled);
		bool NR_FixedJointComponent_IsPreProcessingEnabled(uint64_t entityID);
		void NR_FixedJointComponent_SetPreProcessingEnabled(uint64_t entityID, bool isPreProcessingEnabled);

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

		void* NR_MeshColliderComponent_GetColliderMesh(uint64_t entityID, bool* outIsStatic);
		void NR_MeshColliderComponent_SetColliderMesh(uint64_t entityID, Ref<Mesh>* inMesh);
		void NR_MeshColliderComponent_SetConvex(uint64_t entityID, bool convex);
		bool NR_MeshColliderComponent_IsTrigger(uint64_t entityID);
		void NR_MeshColliderComponent_SetTrigger(uint64_t entityID, bool isTrigger);

		// Renderer
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
		void* NR_Mesh_Constructor(MonoString* filepath, bool* outIsStatic);
		void NR_Mesh_Destructor(void* _this, bool isStatic);
		Ref<MaterialAsset>* NR_Mesh_GetMaterial(void* inMesh, bool isStatic);
		Ref<MaterialAsset>* NR_Mesh_GetMaterialByIndex(void* inMesh, int index, bool isStatic);
		uint32_t NR_Mesh_GetMaterialCount(void* inMesh, bool isStatic);

		void* NR_MeshFactory_CreatePlane(float width, float height);

		// TextComponent
		MonoString* NR_TextComponent_GetText(uint64_t entityID);
		void NR_TextComponent_SetText(uint64_t entityID, MonoString* string);
		void NR_TextComponent_GetColor(uint64_t entityID, glm::vec4* outColor);
		void NR_TextComponent_SetColor(uint64_t entityID, glm::vec4* inColor);

		// Audio
		bool NR_AudioComponent_IsPlaying(uint64_t entityID);
		bool NR_AudioComponent_Play(uint64_t entityID, float startTime = 0.0f);
		bool NR_AudioComponent_Stop(uint64_t entityID);
		bool NR_AudioComponent_Pause(uint64_t entityID);
		bool NR_AudioComponent_Resume(uint64_t entityID);

		float NR_AudioComponent_GetVolumeMult(uint64_t entityID);
		void NR_AudioComponent_SetVolumeMult(uint64_t entityID, float volumeMult);
		float NR_AudioComponent_GetPitchMult(uint64_t entityID);
		void NR_AudioComponent_SetPitchMult(uint64_t entityID, float pitchMult);

		void NR_AudioComponent_SetEvent(uint64_t entityID, Audio::CommandID eventID);

#if OLD_API
		float NR_AudioComponent_GetMasterReverbSend(uint64_t entityID);
		void NR_AudioComponent_SetMasterReverbSend(uint64_t entityID, float sendLevel);
#endif

		uint64_t NR_Audio_CreateSound(Audio::CommandID eventID, Audio::Transform* inSpawnLocation, float volume = 1.0f, float pitch = 1.0f);

		uint32_t NR_Audio_CommandID_Constructor(MonoString* commandName);
		uint32_t NR_Audio_PostEvent(Audio::CommandID eventID, uint64_t objectID);
		uint32_t NR_Audio_PostEventFromAC(Audio::CommandID eventID, uint64_t entityID);
		uint32_t NR_Audio_PostEventAtLocation(Audio::CommandID eventID, Audio::Transform* inSpawnLocation);

		bool NR_Audio_StopEventID(uint32_t playingEvent);
		bool NR_Audio_PauseEventID(uint32_t playingEvent);
		bool NR_Audio_ResumeEventID(uint32_t playingEvent);

		uint64_t NR_AudioObject_Constructor(MonoString* debugName, Audio::Transform* inObjectTransform);
		void NR_ReleaseAudioObject(uint64_t objectID);
		void NR_AudioObject_SetTransform(uint64_t objectID, Audio::Transform* inObjectTransform);
		void NR_AudioObject_GetTransform(uint64_t objectID, Audio::Transform* outObjectTransform);
		bool NR_Audio_GetObjectInfo(uint64_t objectID, MonoString* outDebugName);

		enum class LogLevel : int32_t
		{
			Trace = 1 << 0,
			Debug = 1 << 1,
			Info = 1 << 2,
			Warn = 1 << 3,
			Error = 1 << 4,
			Critical = 1 << 5
		};

		void NR_Log_LogMessage(LogLevel level, MonoString* message);
}