#include "nrpch.h"
#include "ScriptEngineRegistry.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "ScriptWrappers.h"
#include "NotRed/Scene/Entity.h"
#include "NotRed/Audio/AudioComponent.h"

namespace NR 
{
	std::unordered_map<MonoType*, std::function<bool(Entity&)>> sHasComponentFuncs;
	std::unordered_map<MonoType*, std::function<void(Entity&)>> sCreateComponentFuncs;

	extern MonoImage* sCoreAssemblyImage;
	
#define Component_RegisterType(Type) \
	{\
		MonoType* type = mono_reflection_type_from_name((char*)"NR." #Type, sCoreAssemblyImage);\
		if (type) {\
			uint32_t id = mono_type_get_type(type);\
			sHasComponentFuncs[type] = [](Entity& entity) { return entity.HasComponent<Type>(); };\
			sCreateComponentFuncs[type] = [](Entity& entity) { entity.AddComponent<Type>(); };\
		} else {\
			NR_CORE_ERROR("No C# component class found for " #Type "!");\
		}\
	}

	static void InitComponentTypes()
	{
		Component_RegisterType(TagComponent);
		Component_RegisterType(TransformComponent);
		Component_RegisterType(MeshComponent);
		Component_RegisterType(AnimationComponent);
		Component_RegisterType(ScriptComponent);
		Component_RegisterType(CameraComponent);
		Component_RegisterType(PointLightComponent);
		Component_RegisterType(SpriteRendererComponent);
		Component_RegisterType(RigidBody2DComponent);
		Component_RegisterType(BoxCollider2DComponent);
		Component_RegisterType(RigidBodyComponent);
		Component_RegisterType(CharacterControllerComponent);
		Component_RegisterType(FixedJointComponent);
		Component_RegisterType(BoxColliderComponent);
		Component_RegisterType(SphereColliderComponent);
		Component_RegisterType(CapsuleColliderComponent);
		Component_RegisterType(MeshColliderComponent);
		Component_RegisterType(TextComponent);
		Component_RegisterType(AudioListenerComponent);
		Component_RegisterType(AudioComponent);
	}

	void ScriptEngineRegistry::RegisterAll()
	{
		InitComponentTypes();

		mono_add_internal_call("NR.Noise::PerlinNoise_Native", NR::Script::NR_Noise_PerlinNoise);

		mono_add_internal_call("NR.Scene::GetEntities", NR::Script::NR_Scene_GetEntities);

		mono_add_internal_call("NR.Physics::Raycast_Native", NR::Script::NR_Physics_Raycast);
		mono_add_internal_call("NR.Physics::OverlapBox_Native", NR::Script::NR_Physics_OverlapBox);
		mono_add_internal_call("NR.Physics::OverlapCapsule_Native", NR::Script::NR_Physics_OverlapCapsule);
		mono_add_internal_call("NR.Physics::OverlapSphere_Native", NR::Script::NR_Physics_OverlapSphere);
		mono_add_internal_call("NR.Physics::OverlapBoxNonAlloc_Native", NR::Script::NR_Physics_OverlapBoxNonAlloc);
		mono_add_internal_call("NR.Physics::OverlapCapsuleNonAlloc_Native", NR::Script::NR_Physics_OverlapCapsuleNonAlloc);
		mono_add_internal_call("NR.Physics::OverlapSphereNonAlloc_Native", NR::Script::NR_Physics_OverlapSphereNonAlloc);
		mono_add_internal_call("NR.Physics::GetGravity_Native", NR::Script::NR_Physics_GetGravity);
		mono_add_internal_call("NR.Physics::SetGravity_Native", NR::Script::NR_Physics_SetGravity);
		mono_add_internal_call("NR.Physics::AddRadialImpulse_Native", NR::Script::NR_Physics_AddRadialImpulse);

		mono_add_internal_call("NR.Physics::Raycast2D_Native", NR::Script::NR_Physics_Raycast2D);

		mono_add_internal_call("NR.Entity::GetParent_Native", NR::Script::NR_Entity_GetParent);
		mono_add_internal_call("NR.Entity::SetParent_Native", NR::Script::NR_Entity_SetParent);
		mono_add_internal_call("NR.Entity::GetChildren_Native", NR::Script::NR_Entity_GetChildren);
		mono_add_internal_call("NR.Entity::CreateComponent_Native", NR::Script::NR_Entity_CreateComponent);
		mono_add_internal_call("NR.Entity::CreateEntity_Native", NR::Script::NR_Entity_CreateEntity);
		mono_add_internal_call("NR.Entity::Instantiate_Native", NR::Script::NR_Entity_Instantiate);
		mono_add_internal_call("NR.Entity::InstantiateWithPosition_Native", NR::Script::NR_Entity_InstantiateWithPosition);
		mono_add_internal_call("NR.Entity::InstantiateWithTransform_Native", NR::Script::NR_Entity_InstantiateWithTransform);
		mono_add_internal_call("NR.Entity::DestroyEntity_Native", NR::Script::NR_Entity_DestroyEntity);
		mono_add_internal_call("NR.Entity::HasComponent_Native", NR::Script::NR_Entity_HasComponent);
		mono_add_internal_call("NR.Entity::FindEntityByTag_Native", NR::Script::NR_Entity_FindEntityByTag);

		mono_add_internal_call("NR.SceneManager::LoadScene_Native", NR::Script::NR_SceneManager_LoadScene);

		mono_add_internal_call("NR.TagComponent::GetTag_Native", NR::Script::NR_TagComponent_GetTag);
		mono_add_internal_call("NR.TagComponent::SetTag_Native", NR::Script::NR_TagComponent_SetTag);

		mono_add_internal_call("NR.TransformComponent::GetTransform_Native", NR::Script::NR_TransformComponent_GetTransform);
		mono_add_internal_call("NR.TransformComponent::SetTransform_Native", NR::Script::NR_TransformComponent_SetTransform);
		mono_add_internal_call("NR.TransformComponent::GetPosition_Native", NR::Script::NR_TransformComponent_GetPosition);
		mono_add_internal_call("NR.TransformComponent::SetPosition_Native", NR::Script::NR_TransformComponent_SetPosition);
		mono_add_internal_call("NR.TransformComponent::GetRotation_Native", NR::Script::NR_TransformComponent_GetRotation);
		mono_add_internal_call("NR.TransformComponent::SetRotation_Native", NR::Script::NR_TransformComponent_SetRotation);
		mono_add_internal_call("NR.TransformComponent::GetScale_Native", NR::Script::NR_TransformComponent_GetScale);
		mono_add_internal_call("NR.TransformComponent::SetScale_Native", NR::Script::NR_TransformComponent_SetScale);
		mono_add_internal_call("NR.TransformComponent::GetWorldSpaceTransform_Native", NR::Script::NR_TransformComponent_GetWorldSpaceTransform);
		mono_add_internal_call("NR.Transform::TransformMultiply_Native", NR::Script::NR_Transform_TransformMultiply);


		mono_add_internal_call("NR.MeshComponent::GetMesh_Native", NR::Script::NR_MeshComponent_GetMesh);
		mono_add_internal_call("NR.MeshComponent::SetMesh_Native", NR::Script::NR_MeshComponent_SetMesh);
		mono_add_internal_call("NR.MeshComponent::HasMaterial_Native", NR::Script::NR_MeshComponent_HasMaterial);
		mono_add_internal_call("NR.MeshComponent::GetMaterial_Native", NR::Script::NR_MeshComponent_GetMaterial);
		mono_add_internal_call("NR.MeshComponent::GetIsRigged_Native", NR::Script::NR_MeshComponent_GetIsRigged);

		mono_add_internal_call("NR.AnimationComponent::GetIsAnimationPlaying_Native", NR::Script::NR_AnimationComponent_GetIsAnimationPlaying);
		mono_add_internal_call("NR.AnimationComponent::SetIsAnimationPlaying_Native", NR::Script::NR_AnimationComponent_SetIsAnimationPlaying);
		mono_add_internal_call("NR.AnimationComponent::GetStateIndex_Native", NR::Script::NR_AnimationComponent_GetStateIndex);
		mono_add_internal_call("NR.AnimationComponent::SetStateIndex_Native", NR::Script::NR_AnimationComponent_SetStateIndex);
		mono_add_internal_call("NR.AnimationComponent::GetEnableRootMotion_Native", NR::Script::NR_AnimationComponent_GetEnableRootMotion);
		mono_add_internal_call("NR.AnimationComponent::SetEnableRootMotion_Native", NR::Script::NR_AnimationComponent_SetEnableRootMotion);
		mono_add_internal_call("NR.AnimationComponent::GetRootMotion_Native", NR::Script::NR_AnimationComponent_GetRootMotion);

		mono_add_internal_call("NR.ScriptComponent::GetInstance_Native", NR::Script::NR_ScriptComponent_GetInstance);

		mono_add_internal_call("NR.PointLightComponent::GetRadiance_Native", NR::Script::NR_PointLightComponent_GetRadiance);
		mono_add_internal_call("NR.PointLightComponent::SetRadiance_Native", NR::Script::NR_PointLightComponent_SetRadiance);
		mono_add_internal_call("NR.PointLightComponent::GetIntensity_Native", NR::Script::NR_PointLightComponent_GetIntensity);
		mono_add_internal_call("NR.PointLightComponent::SetIntensity_Native", NR::Script::NR_PointLightComponent_SetIntensity);

		mono_add_internal_call("NR.RigidBody2DComponent::GetBodyType_Native", NR::Script::NR_RigidBody2DComponent_GetBodyType);
		mono_add_internal_call("NR.RigidBody2DComponent::SetBodyType_Native", NR::Script::NR_RigidBody2DComponent_SetBodyType);
		mono_add_internal_call("NR.RigidBody2DComponent::GetPosition_Native", NR::Script::NR_RigidBody2DComponent_GetPosition);
		mono_add_internal_call("NR.RigidBody2DComponent::SetPosition_Native", NR::Script::NR_RigidBody2DComponent_SetPosition);
		mono_add_internal_call("NR.RigidBody2DComponent::GetRotation_Native", NR::Script::NR_RigidBody2DComponent_GetRotation);
		mono_add_internal_call("NR.RigidBody2DComponent::SetRotation_Native", NR::Script::NR_RigidBody2DComponent_SetRotation);
		mono_add_internal_call("NR.RigidBody2DComponent::AddForce_Native", NR::Script::NR_RigidBody2DComponent_AddForce);
		mono_add_internal_call("NR.RigidBody2DComponent::AddTorque_Native", NR::Script::NR_RigidBody2DComponent_AddTorque);
		mono_add_internal_call("NR.RigidBody2DComponent::ApplyLinearImpulse_Native", NR::Script::NR_RigidBody2DComponent_ApplyLinearImpulse);
		mono_add_internal_call("NR.RigidBody2DComponent::ApplyAngularImpulse_Native", NR::Script::NR_RigidBody2DComponent_ApplyAngularImpulse);
		mono_add_internal_call("NR.RigidBody2DComponent::GetVelocity_Native", NR::Script::NR_RigidBody2DComponent_GetVelocity);
		mono_add_internal_call("NR.RigidBody2DComponent::SetVelocity_Native", NR::Script::NR_RigidBody2DComponent_SetVelocity);
		mono_add_internal_call("NR.RigidBody2DComponent::GetGravityScale_Native", NR::Script::NR_RigidBody2DComponent_GetGravityScale);
		mono_add_internal_call("NR.RigidBody2DComponent::SetGravityScale_Native", NR::Script::NR_RigidBody2DComponent_SetGravityScale);
		mono_add_internal_call("NR.RigidBody2DComponent::GetMass_Native", NR::Script::NR_RigidBody2DComponent_GetMass);
		mono_add_internal_call("NR.RigidBody2DComponent::SetMass_Native", NR::Script::NR_RigidBody2DComponent_SetMass);

		mono_add_internal_call("NR.RigidBodyComponent::GetBodyType_Native", NR::Script::NR_RigidBodyComponent_GetBodyType);
		mono_add_internal_call("NR.RigidBodyComponent::SetBodyType_Native", NR::Script::NR_RigidBodyComponent_SetBodyType);
		mono_add_internal_call("NR.RigidBodyComponent::GetPosition_Native", NR::Script::NR_RigidBodyComponent_GetPosition);
		mono_add_internal_call("NR.RigidBodyComponent::SetPosition_Native", NR::Script::NR_RigidBodyComponent_SetPosition);
		mono_add_internal_call("NR.RigidBodyComponent::GetRotation_Native", NR::Script::NR_RigidBodyComponent_GetRotation);
		mono_add_internal_call("NR.RigidBodyComponent::SetRotation_Native", NR::Script::NR_RigidBodyComponent_SetRotation);
		mono_add_internal_call("NR.RigidBodyComponent::Rotate_Native", NR::Script::NR_RigidBodyComponent_Rotate);
		mono_add_internal_call("NR.RigidBodyComponent::AddForce_Native", NR::Script::NR_RigidBodyComponent_AddForce);
		mono_add_internal_call("NR.RigidBodyComponent::AddTorque_Native", NR::Script::NR_RigidBodyComponent_AddTorque);
		mono_add_internal_call("NR.RigidBodyComponent::GetVelocity_Native", NR::Script::NR_RigidBodyComponent_GetVelocity);
		mono_add_internal_call("NR.RigidBodyComponent::SetVelocity_Native", NR::Script::NR_RigidBodyComponent_SetVelocity);
		mono_add_internal_call("NR.RigidBodyComponent::GetAngularVelocity_Native", NR::Script::NR_RigidBodyComponent_GetAngularVelocity);
		mono_add_internal_call("NR.RigidBodyComponent::SetAngularVelocity_Native", NR::Script::NR_RigidBodyComponent_SetAngularVelocity);
		mono_add_internal_call("NR.RigidBodyComponent::GetMaxVelocity_Native", NR::Script::NR_RigidBodyComponent_GetMaxVelocity);
		mono_add_internal_call("NR.RigidBodyComponent::SetMaxVelocity_Native", NR::Script::NR_RigidBodyComponent_SetMaxVelocity);
		mono_add_internal_call("NR.RigidBodyComponent::GetMaxAngularVelocity_Native", NR::Script::NR_RigidBodyComponent_GetMaxAngularVelocity);
		mono_add_internal_call("NR.RigidBodyComponent::SetMaxAngularVelocity_Native", NR::Script::NR_RigidBodyComponent_SetMaxAngularVelocity);
		mono_add_internal_call("NR.RigidBodyComponent::GetLinearDrag_Native", NR::Script::NR_RigidBodyComponent_GetLinearDrag);
		mono_add_internal_call("NR.RigidBodyComponent::SetLinearDrag_Native", NR::Script::NR_RigidBodyComponent_SetLinearDrag);
		mono_add_internal_call("NR.RigidBodyComponent::GetAngularDrag_Native", NR::Script::NR_RigidBodyComponent_GetAngularDrag);
		mono_add_internal_call("NR.RigidBodyComponent::SetAngularDrag_Native", NR::Script::NR_RigidBodyComponent_SetAngularDrag);
		mono_add_internal_call("NR.RigidBodyComponent::GetLayer_Native", NR::Script::NR_RigidBodyComponent_GetLayer);
		mono_add_internal_call("NR.RigidBodyComponent::GetMass_Native", NR::Script::NR_RigidBodyComponent_GetMass);
		mono_add_internal_call("NR.RigidBodyComponent::SetMass_Native", NR::Script::NR_RigidBodyComponent_SetMass);
		mono_add_internal_call("NR.RigidBodyComponent::GetKinematicTarget_Native", NR::Script::NR_RigidBodyComponent_GetKinematicTarget);
		mono_add_internal_call("NR.RigidBodyComponent::SetKinematicTarget_Native", NR::Script::NR_RigidBodyComponent_SetKinematicTarget);
		mono_add_internal_call("NR.RigidBodyComponent::SetLockFlag_Native", NR::Script::NR_RigidBodyComponent_ModifyLockFlag);
		mono_add_internal_call("NR.RigidBodyComponent::IsLockFlagSet_Native", NR::Script::NR_RigidBodyComponent_IsLockFlagSet);
		mono_add_internal_call("NR.RigidBodyComponent::GetLockFlags_Native", NR::Script::NR_RigidBodyComponent_GetLockFlags);
		mono_add_internal_call("NR.RigidBodyComponent::IsKinematic_Native", NR::Script::NR_RigidBodyComponent_IsKinematic);
		mono_add_internal_call("NR.RigidBodyComponent::SetIsKinematic_Native", NR::Script::NR_RigidBodyComponent_SetIsKinematic);
		mono_add_internal_call("NR.RigidBodyComponent::IsSleeping_Native", NR::Script::NR_RigidBodyComponent_IsSleeping);
		mono_add_internal_call("NR.RigidBodyComponent::SetIsSleeping_Native", NR::Script::NR_RigidBodyComponent_SetIsSleeping);

		mono_add_internal_call("NR.CharacterControllerComponent::GetSlopeLimit_Native", NR::Script::NR_CharacterControllerComponent_GetSlopeLimit);
		mono_add_internal_call("NR.CharacterControllerComponent::SetSlopeLimit_Native", NR::Script::NR_CharacterControllerComponent_SetSlopeLimit);
		mono_add_internal_call("NR.CharacterControllerComponent::GetStepOffset_Native", NR::Script::NR_CharacterControllerComponent_GetStepOffset);
		mono_add_internal_call("NR.CharacterControllerComponent::SetStepOffset_Native", NR::Script::NR_CharacterControllerComponent_SetStepOffset);
		mono_add_internal_call("NR.CharacterControllerComponent::Move_Native", NR::Script::NR_CharacterControllerComponent_Move);
		mono_add_internal_call("NR.CharacterControllerComponent::Jump_Native", NR::Script::NR_CharacterControllerComponent_Jump);
		mono_add_internal_call("NR.CharacterControllerComponent::GetSpeedDown_Native", NR::Script::NR_CharacterControllerComponent_GetSpeedDown);
		mono_add_internal_call("NR.CharacterControllerComponent::IsGrounded_Native", NR::Script::NR_CharacterControllerComponent_IsGrounded);
		mono_add_internal_call("NR.CharacterControllerComponent::GetCollisionFlags_Native", NR::Script::NR_CharacterControllerComponent_GetCollisionFlags);

		mono_add_internal_call("NR.FixedJointComponent::GetConnectedEntity_Native", NR::Script::NR_FixedJointComponent_GetConnectedEntity);
		mono_add_internal_call("NR.FixedJointComponent::SetConnectedEntity_Native", NR::Script::NR_FixedJointComponent_SetConnectedEntity);
		mono_add_internal_call("NR.FixedJointComponent::IsBreakable_Native", NR::Script::NR_FixedJointComponent_IsBreakable);
		mono_add_internal_call("NR.FixedJointComponent::SetIsBreakable_Native", NR::Script::NR_FixedJointComponent_SetIsBreakable);
		mono_add_internal_call("NR.FixedJointComponent::IsBroken_Native", NR::Script::NR_FixedJointComponent_IsBroken);
		mono_add_internal_call("NR.FixedJointComponent::Break_Native", NR::Script::NR_FixedJointComponent_Break);
		mono_add_internal_call("NR.FixedJointComponent::GetBreakForce_Native", NR::Script::NR_FixedJointComponent_GetBreakForce);
		mono_add_internal_call("NR.FixedJointComponent::SetBreakForce_Native", NR::Script::NR_FixedJointComponent_SetBreakForce);
		mono_add_internal_call("NR.FixedJointComponent::GetBreakTorque_Native", NR::Script::NR_FixedJointComponent_GetBreakTorque);
		mono_add_internal_call("NR.FixedJointComponent::SetBreakTorque_Native", NR::Script::NR_FixedJointComponent_SetBreakTorque);
		mono_add_internal_call("NR.FixedJointComponent::IsCollisionEnabled_Native", NR::Script::NR_FixedJointComponent_IsCollisionEnabled);
		mono_add_internal_call("NR.FixedJointComponent::SetCollisionEnabled_Native", NR::Script::NR_FixedJointComponent_SetCollisionEnabled);
		mono_add_internal_call("NR.FixedJointComponent::IsPreProcessingEnabled_Native", NR::Script::NR_FixedJointComponent_IsPreProcessingEnabled);
		mono_add_internal_call("NR.FixedJointComponent::SetPreProcessingEnabled_Native", NR::Script::NR_FixedJointComponent_SetPreProcessingEnabled);

		mono_add_internal_call("NR.BoxColliderComponent::GetSize_Native", NR::Script::NR_BoxColliderComponent_GetSize);
		mono_add_internal_call("NR.BoxColliderComponent::SetSize_Native", NR::Script::NR_BoxColliderComponent_SetSize);
		mono_add_internal_call("NR.BoxColliderComponent::GetOffset_Native", NR::Script::NR_BoxColliderComponent_GetOffset);
		mono_add_internal_call("NR.BoxColliderComponent::SetOffset_Native", NR::Script::NR_BoxColliderComponent_SetOffset);
		mono_add_internal_call("NR.BoxColliderComponent::IsTrigger_Native", NR::Script::NR_BoxColliderComponent_IsTrigger);
		mono_add_internal_call("NR.BoxColliderComponent::SetTrigger_Native", NR::Script::NR_BoxColliderComponent_SetTrigger);

		mono_add_internal_call("NR.SphereColliderComponent::GetRadius_Native", NR::Script::NR_SphereColliderComponent_GetRadius);
		mono_add_internal_call("NR.SphereColliderComponent::SetRadius_Native", NR::Script::NR_SphereColliderComponent_SetRadius);
		mono_add_internal_call("NR.SphereColliderComponent::GetOffset_Native", NR::Script::NR_SphereColliderComponent_GetOffset);
		mono_add_internal_call("NR.SphereColliderComponent::SetOffset_Native", NR::Script::NR_SphereColliderComponent_SetOffset);
		mono_add_internal_call("NR.SphereColliderComponent::IsTrigger_Native", NR::Script::NR_SphereColliderComponent_IsTrigger);
		mono_add_internal_call("NR.SphereColliderComponent::SetTrigger_Native", NR::Script::NR_SphereColliderComponent_SetTrigger);

		mono_add_internal_call("NR.CapsuleColliderComponent::GetRadius_Native", NR::Script::NR_CapsuleColliderComponent_GetRadius);
		mono_add_internal_call("NR.CapsuleColliderComponent::SetRadius_Native", NR::Script::NR_CapsuleColliderComponent_SetRadius);
		mono_add_internal_call("NR.CapsuleColliderComponent::GetHeight_Native", NR::Script::NR_CapsuleColliderComponent_GetHeight);
		mono_add_internal_call("NR.CapsuleColliderComponent::SetHeight_Native", NR::Script::NR_CapsuleColliderComponent_SetHeight);
		mono_add_internal_call("NR.CapsuleColliderComponent::GetOffset_Native", NR::Script::NR_CapsuleColliderComponent_GetOffset);
		mono_add_internal_call("NR.CapsuleColliderComponent::SetOffset_Native", NR::Script::NR_CapsuleColliderComponent_SetOffset);
		mono_add_internal_call("NR.CapsuleColliderComponent::IsTrigger_Native", NR::Script::NR_CapsuleColliderComponent_IsTrigger);
		mono_add_internal_call("NR.CapsuleColliderComponent::SetTrigger_Native", NR::Script::NR_CapsuleColliderComponent_SetTrigger);

		mono_add_internal_call("NR.MeshColliderComponent::GetColliderMesh_Native", NR::Script::NR_MeshColliderComponent_GetColliderMesh);
		mono_add_internal_call("NR.MeshColliderComponent::SetColliderMesh_Native", NR::Script::NR_MeshColliderComponent_SetColliderMesh);
		mono_add_internal_call("NR.MeshColliderComponent::SetConvex_Native", NR::Script::NR_MeshColliderComponent_SetConvex);
		mono_add_internal_call("NR.MeshColliderComponent::IsTrigger_Native", NR::Script::NR_MeshColliderComponent_IsTrigger);
		mono_add_internal_call("NR.MeshColliderComponent::SetTrigger_Native", NR::Script::NR_MeshColliderComponent_SetTrigger);

		mono_add_internal_call("NR.Input::IsKeyPressed_Native", NR::Script::NR_Input_IsKeyPressed);
		mono_add_internal_call("NR.Input::IsMouseButtonPressed_Native", NR::Script::NR_Input_IsMouseButtonPressed);
		mono_add_internal_call("NR.Input::GetMousePosition_Native", NR::Script::NR_Input_GetMousePosition);
		mono_add_internal_call("NR.Input::SetCursorMode_Native", NR::Script::NR_Input_SetCursorMode);
		mono_add_internal_call("NR.Input::GetCursorMode_Native", NR::Script::NR_Input_GetCursorMode);
		mono_add_internal_call("NR.Input::IsControllerPresent_Native", NR::Script::NR_Input_IsControllerPresent);
		mono_add_internal_call("NR.Input::GetConnectedControllerIDs_Native", NR::Script::NR_Input_GetConnectedControllerIDs);
		mono_add_internal_call("NR.Input::GetControllerName_Native", NR::Script::NR_Input_GetControllerName);
		mono_add_internal_call("NR.Input::IsControllerButtonPressed_Native", NR::Script::NR_Input_IsControllerButtonPressed);
		mono_add_internal_call("NR.Input::GetControllerAxis_Native", NR::Script::NR_Input_GetControllerAxis);
		mono_add_internal_call("NR.Input::GetControllerHat_Native", NR::Script::NR_Input_GetControllerHat);

		mono_add_internal_call("NR.Texture2D::Constructor_Native", NR::Script::NR_Texture2D_Constructor);
		mono_add_internal_call("NR.Texture2D::Destructor_Native", NR::Script::NR_Texture2D_Destructor);
		mono_add_internal_call("NR.Texture2D::SetData_Native", NR::Script::NR_Texture2D_SetData);

		mono_add_internal_call("NR.Material::Destructor_Native", NR::Script::NR_Material_Destructor);
		mono_add_internal_call("NR.Material::GetAlbedoColor_Native", NR::Script::NR_Material_GetAlbedoColor);
		mono_add_internal_call("NR.Material::SetAlbedoColor_Native", NR::Script::NR_Material_SetAlbedoColor);
		mono_add_internal_call("NR.Material::GetMetalness_Native", NR::Script::NR_Material_GetMetalness);
		mono_add_internal_call("NR.Material::SetMetalness_Native", NR::Script::NR_Material_SetMetalness);
		mono_add_internal_call("NR.Material::GetRoughness_Native", NR::Script::NR_Material_GetRoughness);
		mono_add_internal_call("NR.Material::SetRoughness_Native", NR::Script::NR_Material_SetRoughness);
		mono_add_internal_call("NR.Material::GetEmission_Native", NR::Script::NR_Material_GetEmission);
		mono_add_internal_call("NR.Material::SetEmission_Native", NR::Script::NR_Material_SetEmission);
		mono_add_internal_call("NR.Material::SetFloat_Native", NR::Script::NR_Material_SetFloat);
		mono_add_internal_call("NR.Material::SetTexture_Native", NR::Script::NR_Material_SetTexture);

		mono_add_internal_call("NR.Mesh::Constructor_Native", NR::Script::NR_Mesh_Constructor);
		mono_add_internal_call("NR.Mesh::Destructor_Native", NR::Script::NR_Mesh_Destructor);
		mono_add_internal_call("NR.Mesh::GetMaterial_Native", NR::Script::NR_Mesh_GetMaterial);
		mono_add_internal_call("NR.Mesh::GetMaterialByIndex_Native", NR::Script::NR_Mesh_GetMaterialByIndex);
		mono_add_internal_call("NR.Mesh::GetMaterialCount_Native", NR::Script::NR_Mesh_GetMaterialCount);

		mono_add_internal_call("NR.MeshFactory::CreatePlane_Native", NR::Script::NR_MeshFactory_CreatePlane);

		mono_add_internal_call("NR.TextComponent::GetText_Native", NR::Script::NR_TextComponent_GetText);
		mono_add_internal_call("NR.TextComponent::SetText_Native", NR::Script::NR_TextComponent_SetText);
		mono_add_internal_call("NR.TextComponent::GetColor_Native", NR::Script::NR_TextComponent_GetColor);
		mono_add_internal_call("NR.TextComponent::SetColor_Native", NR::Script::NR_TextComponent_SetColor);

		mono_add_internal_call("NR.AudioComponent::IsPlaying_Native", NR::Script::NR_AudioComponent_IsPlaying);
		mono_add_internal_call("NR.AudioComponent::Play_Native", NR::Script::NR_AudioComponent_Play);
		mono_add_internal_call("NR.AudioComponent::Stop_Native", NR::Script::NR_AudioComponent_Stop);
		mono_add_internal_call("NR.AudioComponent::Pause_Native", NR::Script::NR_AudioComponent_Pause);
		mono_add_internal_call("NR.AudioComponent::Resume_Native", NR::Script::NR_AudioComponent_Resume);

		mono_add_internal_call("NR.AudioComponent::GetVolumeMult_Native", NR::Script::NR_AudioComponent_GetVolumeMult);
		mono_add_internal_call("NR.AudioComponent::SetVolumeMult_Native", NR::Script::NR_AudioComponent_SetVolumeMult);
		mono_add_internal_call("NR.AudioComponent::GetPitchMult_Native", NR::Script::NR_AudioComponent_GetPitchMult);
		mono_add_internal_call("NR.AudioComponent::SetPitchMult_Native", NR::Script::NR_AudioComponent_SetPitchMult);

		mono_add_internal_call("NR.AudioComponent::SetEvent_Native", NR::Script::NR_AudioComponent_SetEvent);
#if OLD_API
		mono_add_internal_call("NR.AudioComponent::GetMasterReverbSend_Native", NR::Script::NR_AudioComponent_GetMasterReverbSend);
		mono_add_internal_call("NR.AudioComponent::SetMasterReverbSend_Native", NR::Script::NR_AudioComponent_SetMasterReverbSend);
#endif		
		mono_add_internal_call("NR.Audio::CreateSoundAtLocationAssetPath_Native", NR::Script::NR_Audio_CreateSound);

		mono_add_internal_call("NR.Audio/CommandID::Constructor_Native", NR::Script::NR_Audio_CommandID_Constructor);
		mono_add_internal_call("NR.Audio::PostEvent_Native", NR::Script::NR_Audio_PostEvent);
		mono_add_internal_call("NR.Audio::PostEventFromAC_Native", NR::Script::NR_Audio_PostEventFromAC);
		mono_add_internal_call("NR.Audio::PostEventAtLocation_Native", NR::Script::NR_Audio_PostEventAtLocation);

		mono_add_internal_call("NR.Audio::StopEventID_Native", NR::Script::NR_Audio_StopEventID);
		mono_add_internal_call("NR.Audio::PauseEventID_Native", NR::Script::NR_Audio_PauseEventID);
		mono_add_internal_call("NR.Audio::ResumeEventID_Native", NR::Script::NR_Audio_ResumeEventID);

		mono_add_internal_call("NR.Audio/Object::Constructor_Native", NR::Script::NR_AudioObject_Constructor);
		mono_add_internal_call("NR.Audio/Object::SetTransform_Native", NR::Script::NR_AudioObject_SetTransform);
		mono_add_internal_call("NR.Audio/Object::GetTransform_Native", NR::Script::NR_AudioObject_GetTransform);
		mono_add_internal_call("NR.Audio::ReleaseAudioObject_Native", NR::Script::NR_ReleaseAudioObject);
		mono_add_internal_call("NR.Audio::GetObjectInfo_Native", NR::Script::NR_Audio_GetObjectInfo);

		mono_add_internal_call("NR.Log::LogMessage_Native", NR::Script::NR_Log_LogMessage);
	}

}