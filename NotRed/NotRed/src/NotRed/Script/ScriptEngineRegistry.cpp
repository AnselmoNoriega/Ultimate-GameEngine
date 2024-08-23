#include "nrpch.h"
#include "ScriptEngineRegistry.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "NotRed/Scene/Entity.h"
#include "ScriptWrappers.h"
#include <iostream>

namespace NR {

	std::unordered_map<MonoType*, std::function<bool(Entity&)>> sHasComponentFuncs;
	std::unordered_map<MonoType*, std::function<void(Entity&)>> sCreateComponentFuncs;

	extern MonoImage* s_CoreAssemblyImage;

#define Component_RegisterType(Type) \
	{\
		MonoType* type = mono_reflection_type_from_name("NR." #Type, s_CoreAssemblyImage);\
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
		Component_RegisterType(ScriptComponent);
		Component_RegisterType(CameraComponent);
		Component_RegisterType(SpriteRendererComponent);
	}

	void ScriptEngineRegistry::RegisterAll()
	{
		InitComponentTypes();

		mono_add_internal_call("NR.Noise::PerlinNoise_Native", NR::Script::NR_Noise_PerlinNoise);

		mono_add_internal_call("NR.Entity::GetTransform_Native", NR::Script::NR_Entity_GetTransform);
		mono_add_internal_call("NR.Entity::SetTransform_Native", NR::Script::NR_Entity_SetTransform);
		mono_add_internal_call("NR.Entity::CreateComponent_Native", NR::Script::NR_Entity_CreateComponent);
		mono_add_internal_call("NR.Entity::HasComponent_Native", NR::Script::NR_Entity_HasComponent);

		mono_add_internal_call("NR.MeshComponent::GetMesh_Native", NR::Script::NR_MeshComponent_GetMesh);
		mono_add_internal_call("NR.MeshComponent::SetMesh_Native", NR::Script::NR_MeshComponent_SetMesh);

		mono_add_internal_call("NR.Input::IsKeyPressed_Native", NR::Script::NR_Input_IsKeyPressed);

		mono_add_internal_call("NR.Texture2D::Constructor_Native", NR::Script::NR_Texture2D_Constructor);
		mono_add_internal_call("NR.Texture2D::Destructor_Native", NR::Script::NR_Texture2D_Destructor);
		mono_add_internal_call("NR.Texture2D::SetData_Native", NR::Script::NR_Texture2D_SetData);

		mono_add_internal_call("NR.Material::Destructor_Native", NR::Script::NR_Material_Destructor);
		mono_add_internal_call("NR.Material::SetFloat_Native", NR::Script::NR_Material_SetFloat);
		mono_add_internal_call("NR.Material::SetTexture_Native", NR::Script::NR_Material_SetTexture);

		mono_add_internal_call("NR.MaterialInstance::Destructor_Native", NR::Script::NR_MaterialInstance_Destructor);
		mono_add_internal_call("NR.MaterialInstance::SetFloat_Native", NR::Script::NR_MaterialInstance_SetFloat);
		mono_add_internal_call("NR.MaterialInstance::SetVector3_Native", NR::Script::NR_MaterialInstance_SetVector3);
		mono_add_internal_call("NR.MaterialInstance::SetTexture_Native", NR::Script::NR_MaterialInstance_SetTexture);

		mono_add_internal_call("NR.Mesh::Constructor_Native", NR::Script::NR_Mesh_Constructor);
		mono_add_internal_call("NR.Mesh::Destructor_Native", NR::Script::NR_Mesh_Destructor);
		mono_add_internal_call("NR.Mesh::GetMaterial_Native", NR::Script::NR_Mesh_GetMaterial);
		mono_add_internal_call("NR.Mesh::GetMaterialByIndex_Native", NR::Script::NR_Mesh_GetMaterialByIndex);
		mono_add_internal_call("NR.Mesh::GetMaterialCount_Native", NR::Script::NR_Mesh_GetMaterialCount);

		mono_add_internal_call("NR.MeshFactory::CreatePlane_Native", NR::Script::NR_MeshFactory_CreatePlane);
	}

}