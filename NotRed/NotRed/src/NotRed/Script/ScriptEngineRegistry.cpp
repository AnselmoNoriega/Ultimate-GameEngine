#include "nrpch.h"
#include "ScriptEngineRegistry.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "NotRed/Scene/Entity.h"
#include "ScriptWrappers.h"

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
		Component_RegisterType(ParticleComponent);
		Component_RegisterType(ScriptComponent);
		Component_RegisterType(CameraComponent);
		Component_RegisterType(SpriteRendererComponent);
		Component_RegisterType(RigidBody2DComponent);
		Component_RegisterType(BoxCollider2DComponent);
		Component_RegisterType(RigidBodyComponent);
		Component_RegisterType(BoxColliderComponent);
		Component_RegisterType(SphereColliderComponent);
		Component_RegisterType(AudioListenerComponent);
		Component_RegisterType(Audio::AudioComponent);
	}

	void ScriptEngineRegistry::RegisterAll()
	{
		InitComponentTypes();

		mono_add_internal_call("NR.Noise::PerlinNoise_Native", NR::Script::NR_Noise_PerlinNoise);

		mono_add_internal_call("NR.Physics::Raycast_Native", NR::Script::NR_Physics_Raycast);
		mono_add_internal_call("NR.Physics::OverlapBox_Native", NR::Script::NR_Physics_OverlapBox);
		mono_add_internal_call("NR.Physics::OverlapSphere_Native", NR::Script::NR_Physics_OverlapSphere);
		mono_add_internal_call("NR.Physics::OverlapCapsule_Native", NR::Script::NR_Physics_OverlapCapsule);
		mono_add_internal_call("NR.Physics::OverlapBoxNonAlloc_Native", NR::Script::NR_Physics_OverlapBoxNonAlloc);
		mono_add_internal_call("NR.Physics::OverlapCapsuleNonAlloc_Native", NR::Script::NR_Physics_OverlapCapsuleNonAlloc);
		mono_add_internal_call("NR.Physics::OverlapSphereNonAlloc_Native", NR::Script::NR_Physics_OverlapSphereNonAlloc);
		mono_add_internal_call("NR.TransformComponent::GetTranslation_Native", NR::Script::NR_TransformComponent_GetTranslation);
		mono_add_internal_call("NR.TransformComponent::SetTranslation_Native", NR::Script::NR_TransformComponent_SetTranslation);
		mono_add_internal_call("NR.TransformComponent::GetRotation_Native", NR::Script::NR_TransformComponent_GetRotation);
		mono_add_internal_call("NR.TransformComponent::SetRotation_Native", NR::Script::NR_TransformComponent_SetRotation);
		mono_add_internal_call("NR.TransformComponent::GetScale_Native", NR::Script::NR_TransformComponent_GetScale);
		mono_add_internal_call("NR.TransformComponent::SetScale_Native", NR::Script::NR_TransformComponent_SetScale);
		mono_add_internal_call("NR.TransformComponent::GetWorldSpaceTransform_Native", NR::Script::NR_TransformComponent_GetWorldSpaceTransform);

		mono_add_internal_call("NR.Entity::CreateComponent_Native", NR::Script::NR_Entity_CreateComponent);
		mono_add_internal_call("NR.Entity::CreateEntity_Native", NR::Script::NR_Entity_CreateEntity);
		mono_add_internal_call("NR.Entity::HasComponent_Native", NR::Script::NR_Entity_HasComponent);

		mono_add_internal_call("NR.TransformComponent::GetTransform_Native", NR::Script::NR_TransformComponent_GetTransform);
		mono_add_internal_call("NR.TransformComponent::SetTransform_Native", NR::Script::NR_TransformComponent_SetTransform);

		mono_add_internal_call("NR.Entity::FindEntityByTag_Native", NR::Script::NR_Entity_FindEntityByTag);
		mono_add_internal_call("NR.Entity::InstantiateEntity_Native", NR::Script::NR_Entity_InstantiateEntity);

		mono_add_internal_call("NR.TagComponent::GetTag_Native", NR::Script::NR_TagComponent_GetTag);
		mono_add_internal_call("NR.TagComponent::SetTag_Native", NR::Script::NR_TagComponent_SetTag);

		mono_add_internal_call("NR.MeshComponent::GetMesh_Native", NR::Script::NR_MeshComponent_GetMesh);
		mono_add_internal_call("NR.MeshComponent::SetMesh_Native", NR::Script::NR_MeshComponent_SetMesh);

		mono_add_internal_call("NR.Input::IsKeyPressed_Native", NR::Script::NR_Input_IsKeyPressed);
		mono_add_internal_call("NR.Input::IsMouseButtonPressed_Native", NR::Script::NR_Input_IsMouseButtonPressed);
		mono_add_internal_call("NR.Input::GetMousePosition_Native", NR::Script::NR_Input_GetMousePosition);
		mono_add_internal_call("NR.Input::SetCursorMode_Native", NR::Script::NR_Input_SetCursorMode);
		mono_add_internal_call("NR.Input::GetCursorMode_Native", NR::Script::NR_Input_GetCursorMode);

		mono_add_internal_call("NR.Texture2D::Constructor_Native", NR::Script::NR_Texture2D_Constructor);
		mono_add_internal_call("NR.Texture2D::Destructor_Native", NR::Script::NR_Texture2D_Destructor);
		mono_add_internal_call("NR.Texture2D::SetData_Native", NR::Script::NR_Texture2D_SetData);

		mono_add_internal_call("NR.Material::Destructor_Native", NR::Script::NR_Material_Destructor);
		mono_add_internal_call("NR.Material::SetFloat_Native", NR::Script::NR_Material_SetFloat);
		mono_add_internal_call("NR.Material::SetTexture_Native", NR::Script::NR_Material_SetTexture);

		mono_add_internal_call("NR.MaterialInstance::Destructor_Native", NR::Script::NR_MaterialInstance_Destructor);
		mono_add_internal_call("NR.MaterialInstance::SetFloat_Native", NR::Script::NR_MaterialInstance_SetFloat);
		mono_add_internal_call("NR.MaterialInstance::SetVector3_Native", NR::Script::NR_MaterialInstance_SetVector3);
		mono_add_internal_call("NR.MaterialInstance::SetVector4_Native", NR::Script::NR_MaterialInstance_SetVector4);
		mono_add_internal_call("NR.MaterialInstance::SetTexture_Native", NR::Script::NR_MaterialInstance_SetTexture);

		mono_add_internal_call("NR.Mesh::Constructor_Native", NR::Script::NR_Mesh_Constructor);
		mono_add_internal_call("NR.Mesh::Destructor_Native", NR::Script::NR_Mesh_Destructor);
		mono_add_internal_call("NR.Mesh::GetMaterial_Native", NR::Script::NR_Mesh_GetMaterial);
		mono_add_internal_call("NR.Mesh::GetMaterialByIndex_Native", NR::Script::NR_Mesh_GetMaterialByIndex);
		mono_add_internal_call("NR.Mesh::GetMaterialCount_Native", NR::Script::NR_Mesh_GetMaterialCount);

		mono_add_internal_call("NR.RigidBody2DComponent::GetBodyType_Native", NR::Script::NR_RigidBody2DComponent_GetBodyType);
		mono_add_internal_call("NR.RigidBody2DComponent::SetBodyType_Native", NR::Script::NR_RigidBody2DComponent_SetBodyType);
		mono_add_internal_call("NR.RigidBody2DComponent::ApplyImpulse_Native", NR::Script::NR_RigidBody2DComponent_ApplyImpulse);
		mono_add_internal_call("NR.RigidBody2DComponent::GetVelocity_Native", NR::Script::NR_RigidBody2DComponent_GetVelocity);
		mono_add_internal_call("NR.RigidBody2DComponent::SetVelocity_Native", NR::Script::NR_RigidBody2DComponent_SetVelocity);
		mono_add_internal_call("NR.RigidBodyComponent::Rotate_Native", NR::Script::NR_RigidBodyComponent_Rotate);
		mono_add_internal_call("NR.RigidBodyComponent::GetLayer_Native", NR::Script::NR_RigidBodyComponent_GetLayer);
		mono_add_internal_call("NR.RigidBodyComponent::GetMass_Native", NR::Script::NR_RigidBodyComponent_GetMass);
		mono_add_internal_call("NR.RigidBodyComponent::SetMass_Native", NR::Script::NR_RigidBodyComponent_SetMass);

		mono_add_internal_call("NR.RigidBodyComponent::GetBodyType_Native", NR::Script::NR_RigidBodyComponent_GetBodyType);
		mono_add_internal_call("NR.RigidBodyComponent::SetBodyType_Native", NR::Script::NR_RigidBodyComponent_SetBodyType);
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

		mono_add_internal_call("NR.MeshFactory::CreatePlane_Native", NR::Script::NR_MeshFactory_CreatePlane);

		mono_add_internal_call("NR.AudioComponent::IsPlaying_Native", Script::NR_AudioComponent_IsPlaying);
		mono_add_internal_call("NR.AudioComponent::Play_Native", Script::NR_AudioComponent_Play);
		mono_add_internal_call("NR.AudioComponent::Stop_Native", Script::NR_AudioComponent_Stop);
		mono_add_internal_call("NR.AudioComponent::Pause_Native", Script::NR_AudioComponent_Pause);
		mono_add_internal_call("NR.AudioComponent::GetVolumeMult_Native", Script::NR_AudioComponent_GetVolumeMult);
		mono_add_internal_call("NR.AudioComponent::SetVolumeMult_Native", Script::NR_AudioComponent_SetVolumeMult);
		mono_add_internal_call("NR.AudioComponent::GetPitchMult_Native", Script::NR_AudioComponent_GetPitchMult);
		mono_add_internal_call("NR.AudioComponent::SetPitchMult_Native", Script::NR_AudioComponent_SetPitchMult);
		mono_add_internal_call("NR.AudioComponent::GetLooping_Native", Script::NR_AudioComponent_GetLooping);
		mono_add_internal_call("NR.AudioComponent::SetLooping_Native", Script::NR_AudioComponent_SetLooping);
		mono_add_internal_call("NR.AudioComponent::GetMasterReverbSend_Native", NR::Script::NR_AudioComponent_GetMasterReverbSend);
		mono_add_internal_call("NR.AudioComponent::SetMasterReverbSend_Native", NR::Script::NR_AudioComponent_SetMasterReverbSend);
		mono_add_internal_call("NR.AudioComponent::SetSound_Native", Script::NR_AudioComponent_SetSound);
		mono_add_internal_call("NR.AudioComponent::SetSoundPath_Native", Script::NR_AudioComponent_SetSoundPath);
		mono_add_internal_call("NR.AudioComponent::GetSound_Native", Script::NR_AudioComponent_GetSound);

		mono_add_internal_call("NR.Audio::PlaySound2DAsset_Native", Script::NR_Audio_PlaySound2DAsset);
		mono_add_internal_call("NR.Audio::PlaySound2DAssetPath_Native", Script::NR_Audio_PlaySound2DAssetPath);
		mono_add_internal_call("NR.Audio::PlaySoundAtLocationAsset_Native", Script::NR_Audio_PlaySoundAtLocationAsset);
		mono_add_internal_call("NR.Audio::PlaySoundAtLocationAssetPath_Native", Script::NR_Audio_PlaySoundAtLocationAssetPath);

		mono_add_internal_call("NR.SimpleSound::Constructor_Native", Script::NR_SimpleSound_Constructor);
		mono_add_internal_call("NR.SimpleSound::Destructor_Native", Script::NR_SimpleSound_Destructor);

		mono_add_internal_call("NR.Audio::CreateSound2DAsset_Native", Script::NR_AudioCreateSound2DAsset);
		mono_add_internal_call("NR.Audio::CreateSound2DAssetPath_Native", Script::NR_AudioCreateSound2DPath);
		mono_add_internal_call("NR.Audio::CreateSoundAtLocationAsset_Native", Script::NR_AudioCreateSound3DAsset);
		mono_add_internal_call("NR.Audio::CreateSoundAtLocationAssetPath_Native", Script::NR_AudioCreateSound3DPath);
	}
}