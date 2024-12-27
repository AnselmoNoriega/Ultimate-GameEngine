#include "nrpch.h"
#include "ScriptWrappers.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/common.hpp>

#include <mono/jit/jit.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <box2d/box2d.h>

#include <PxPhysicsAPI.h>

#include "NotRed/Core/Application.h"

#include "NotRed/Asset/AssetManager.h"
#include "NotRed/Physics/PhysicsManager.h"
#include "NotRed/Physics/PhysicsActor.h"

#include "NotRed/Math/Noise.h"

#include "NotRed/Scene/Scene.h"
#include "NotRed/Scene/Entity.h"

#include "NotRed/Audio/AudioComponent.h"
#include "NotRed/Audio/AudioPlayback.h"

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
		bool wasPressed = Input::IsMouseButtonPressed(button);
		bool enableImGui = Application::Get().GetSpecification().EnableImGui;
		if (wasPressed && enableImGui && GImGui->HoveredWindow != nullptr)
		{
			// Make sure we're in the viewport panel
			ImGuiWindow* viewportWindow = ImGui::FindWindowByName("Viewport");
			if (viewportWindow != nullptr)
			{
				wasPressed = GImGui->HoveredWindow->ID == viewportWindow->ID;
			}
		}
		return wasPressed;
	}

	////////////////////////////////////////////////////////////////
	// Entity //////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////


	uint64_t NR_Entity_GetParent(uint64_t entityID)
	{
		Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
		NR_CORE_ASSERT(scene, "No active scene!");
		
		const auto& entityMap = scene->GetEntityMap();
		NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		return entityMap.at(entityID).GetParentID();
	}

	MonoArray* NR_Entity_GetChildren(uint64_t entityID)
	{
		Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
		NR_CORE_ASSERT(scene, "No active scene!");
	
		const auto& entityMap = scene->GetEntityMap();
		NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");
		
		Entity entity = entityMap.at(entityID);
		const auto& children = entity.Children();
		MonoArray* result = mono_array_new(mono_domain_get(), ScriptEngine::GetCoreClass("NR.Entity"), children.size());
		uint32_t index = 0;
		
		for (auto child : children)
		{
			void* data[] = {
				&child
			};
			MonoObject* obj = ScriptEngine::Construct("NR.Entity:.ctor(ulong)", true, data);
			mono_array_set(result, MonoObject*, index++, obj);
		}
		
		return result;
	}

	uint64_t NR_Entity_CreateEntity(uint64_t entityID)
	{
		Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
		NR_CORE_ASSERT(scene, "No active scene!");
		const auto& entityMap = scene->GetEntityMap();
		NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");
		Entity result = scene->CreateEntity("Unnamed from C#");
		return result.GetID();
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

	uint64_t NR_Entity_InstantiateEntity()
	{
		Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
		NR_CORE_ASSERT(scene, "No active scene!");

		Entity entity = scene->CreateEntity();
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

	void NR_TransformComponent_GetWorldSpaceTransform(uint64_t entityID, TransformComponent* outTransform)
	{
		Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
		NR_CORE_ASSERT(scene, "No active scene!");

		const auto& entityMap = scene->GetEntityMap();
		NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = entityMap.at(entityID);
		*outTransform = scene->GetWorldSpaceTransform(entity);
	}

	MonoString* NR_TagComponent_GetTag(uint64_t entityID)
	{
		Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
		NR_CORE_ASSERT(scene, "No active scene!");

		const auto& entityMap = scene->GetEntityMap();
		NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = entityMap.at(entityID);
		auto& tagComponent = entity.GetComponent<TagComponent>();
		return mono_string_new(mono_domain_get(), tagComponent.Tag.c_str());
	}

	void NR_TagComponent_SetTag(uint64_t entityID, MonoString* tag)
	{
		Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
		NR_CORE_ASSERT(scene, "No active scene!");

		const auto& entityMap = scene->GetEntityMap();
		NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = entityMap.at(entityID);
		auto& tagComponent = entity.GetComponent<TagComponent>();
		tagComponent.Tag = mono_string_to_utf8(tag);
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

	bool NR_Physics_RaycastWithStruct(RaycastData* inData, RaycastHit* hit)
	{
		return NR_Physics_Raycast(&inData->Origin, &inData->Direction, inData->MaxDistance, inData->RequiredComponentTypes, hit);
	}

	bool NR_Physics_Raycast(glm::vec3* origin, glm::vec3* direction, float maxDistance, MonoArray* requiredComponentTypes, RaycastHit* hit)
	{
		Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
		NR_CORE_ASSERT(scene, "No active scene!");
		NR_CORE_ASSERT(PhysicsManager::GetScene()->IsValid());

		RaycastHit temp;
		bool success = PhysicsManager::GetScene()->Raycast(*origin, *direction, maxDistance, &temp);
		if (success && requiredComponentTypes != nullptr)
		{
			Entity entity = scene->FindEntityByID(temp.HitEntity);
			size_t length = mono_array_length(requiredComponentTypes);
			for (size_t i = 0; i < length; ++i)
			{
				void* rawType = mono_array_get(requiredComponentTypes, void*, i);
				if (rawType == nullptr)
				{
					NR_CONSOLE_LOG_ERROR("Why did you feel the need to pass a \"null\" System.Type to Physics.Raycast?");
					success = false;
					break;
				}
				MonoType* type = mono_reflection_type_get_type((MonoReflectionType*)rawType);

#ifdef NR_DEBUG
				MonoClass* typeClass = mono_type_get_class(type);
				MonoClass* parentClass = mono_class_get_parent(typeClass);
				bool validComponentFilter = true;
				if (parentClass == nullptr)
				{
					validComponentFilter = false;
				}
				else
				{
					const char* parentClassName = mono_class_get_name(parentClass);
					const char* parentNamespace = mono_class_get_namespace(parentClass);
					if (strstr(parentClassName, "Component") == nullptr || strstr(parentNamespace, "NR") == nullptr)
						validComponentFilter = false;
				}
				if (!validComponentFilter)
				{
					NR_CONSOLE_LOG_ERROR("{0} does not inherit from NR.Component!", mono_class_get_name(typeClass));
					success = false;
					break;
				}
#endif

				if (!sHasComponentFuncs[type](entity))
				{
					success = false;
					break;
				}
			}
		}

		if (!success)
		{
			temp.HitEntity = 0;
			temp.Distance = 0.0f;
			temp.Normal = glm::vec3(0.0f);
			temp.Position = glm::vec3(0.0f);
		}

		*hit = temp;
		return success;
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
		NR_CORE_ASSERT(PhysicsManager::GetScene()->IsValid());

		MonoArray* outColliders = nullptr;
		memset(sOverlapBuffer.data(), 0, OVERLAP_MAX_COLLIDERS * sizeof(physx::PxOverlapHit));

		uint32_t count;
		if (PhysicsManager::GetScene()->OverlapBox(*origin, *halfSize, sOverlapBuffer, count))
		{
			outColliders = mono_array_new(mono_domain_get(), ScriptEngine::GetCoreClass("NR.Collider"), count);
			AddCollidersToArray(outColliders, sOverlapBuffer, count, count);
		}

		return outColliders;
	}

	MonoArray* NR_Physics_OverlapCapsule(glm::vec3* origin, float radius, float halfHeight)
	{
		NR_CORE_ASSERT(PhysicsManager::GetScene()->IsValid());

		MonoArray* outColliders = nullptr;
		memset(sOverlapBuffer.data(), 0, OVERLAP_MAX_COLLIDERS * sizeof(physx::PxOverlapHit));

		uint32_t count;
		if (PhysicsManager::GetScene()->OverlapCapsule(*origin, radius, halfHeight, sOverlapBuffer, count))
		{
			outColliders = mono_array_new(mono_domain_get(), ScriptEngine::GetCoreClass("NR.Collider"), count);
			AddCollidersToArray(outColliders, sOverlapBuffer, count, count);
		}
		return outColliders;
	}

	int32_t NR_Physics_OverlapBoxNonAlloc(glm::vec3* origin, glm::vec3* halfSize, MonoArray* outColliders)
	{
		NR_CORE_ASSERT(PhysicsManager::GetScene()->IsValid());

		memset(sOverlapBuffer.data(), 0, OVERLAP_MAX_COLLIDERS * sizeof(physx::PxOverlapHit));

		const uint32_t arrayLength = (uint32_t)mono_array_length(outColliders);

		uint32_t count;
		if (PhysicsManager::GetScene()->OverlapBox(*origin, *halfSize, sOverlapBuffer, count))
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
		NR_CORE_ASSERT(PhysicsManager::GetScene()->IsValid());

		memset(sOverlapBuffer.data(), 0, OVERLAP_MAX_COLLIDERS * sizeof(physx::PxOverlapHit));

		const uint32_t arrayLength = (uint32_t)mono_array_length(outColliders);

		uint32_t count = 0;
		if (PhysicsManager::GetScene()->OverlapCapsule(*origin, radius, halfHeight, sOverlapBuffer, count))
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
		NR_CORE_ASSERT(PhysicsManager::GetScene()->IsValid());

		memset(sOverlapBuffer.data(), 0, OVERLAP_MAX_COLLIDERS * sizeof(physx::PxOverlapHit));

		const uint32_t arrayLength = (uint32_t)mono_array_length(outColliders);

		uint32_t count = 0;
		if (PhysicsManager::GetScene()->OverlapSphere(*origin, radius, sOverlapBuffer, count))
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

	void NR_RigidBody2DComponent_GetBodyType(uint64_t entityID, RigidBody2DComponent::Type* outType)
	{
		Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();

		NR_CORE_ASSERT(scene, "No active scene!");
		const auto& entityMap = scene->GetEntityMap();

		NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");
		Entity entity = entityMap.at(entityID);

		NR_CORE_ASSERT(entity.HasComponent<RigidBody2DComponent>());
		auto& component = entity.GetComponent<RigidBody2DComponent>();
		*outType = component.BodyType;
	}

	void NR_RigidBody2DComponent_SetBodyType(uint64_t entityID, RigidBody2DComponent::Type* type)
	{
		Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();

		NR_CORE_ASSERT(scene, "No active scene!");
		const auto& entityMap = scene->GetEntityMap();

		NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");
		Entity entity = entityMap.at(entityID);

		NR_CORE_ASSERT(entity.HasComponent<RigidBody2DComponent>());
		auto& component = entity.GetComponent<RigidBody2DComponent>();
		component.BodyType = *type;
	}

	void NR_RigidBodyComponent_GetPosition(uint64_t entityID, glm::vec3* position)
	{
		Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
		NR_CORE_ASSERT(scene, "No active scene!");

		const auto& entityMap = scene->GetEntityMap();
		NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = entityMap.at(entityID);
		NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

		Ref<PhysicsActor> actor = PhysicsManager::GetScene()->GetActor(entity);
		*position = actor->GetPosition();
	}

	void NR_RigidBodyComponent_SetPosition(uint64_t entityID, glm::vec3* position)
	{
		Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
		NR_CORE_ASSERT(scene, "No active scene!");

		const auto& entityMap = scene->GetEntityMap();
		NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = entityMap.at(entityID);
		NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

		Ref<PhysicsActor> actor = PhysicsManager::GetScene()->GetActor(entity);
		actor->SetPosition(*position);
	}

	void NR_RigidBodyComponent_GetRotation(uint64_t entityID, glm::vec3* rotation)
	{
		Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
		NR_CORE_ASSERT(scene, "No active scene!");

		const auto& entityMap = scene->GetEntityMap();
		NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = entityMap.at(entityID);
		NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

		auto& component = entity.GetComponent<RigidBodyComponent>();
		NR_CORE_ASSERT(rotation);

		Ref<PhysicsActor> actor = PhysicsManager::GetScene()->GetActor(entity);
		*rotation = actor->GetRotation();
	}

	void NR_RigidBodyComponent_SetRotation(uint64_t entityID, glm::vec3* rotation)
	{
		Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
		NR_CORE_ASSERT(scene, "No active scene!");

		const auto& entityMap = scene->GetEntityMap();
		NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = entityMap.at(entityID);
		NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

		auto& component = entity.GetComponent<RigidBodyComponent>();
		NR_CORE_ASSERT(rotation);

		Ref<PhysicsActor> actor = PhysicsManager::GetScene()->GetActor(entity);
		actor->SetRotation(*rotation);
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

		Ref<PhysicsActor> actor = PhysicsManager::GetScene()->GetActor(entity);
		actor->Rotate(*rotation);
	}

	void NR_RigidBody2DComponent_ApplyImpulse(uint64_t entityID, glm::vec2* impulse, glm::vec2* offset, bool wake)
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

	void NR_RigidBody2DComponent_GetVelocity(uint64_t entityID, glm::vec2* outVelocity)
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

	void NR_RigidBody2DComponent_SetVelocity(uint64_t entityID, glm::vec2* velocity)
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

	void NR_RigidBodyComponent_SetBodyType(uint64_t entityID, RigidBodyComponent::Type type)
	{
		Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
		NR_CORE_ASSERT(scene, "No active scene!");
		const auto& entityMap = scene->GetEntityMap();
		NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = entityMap.at(entityID);
		NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
		auto& component = entity.GetComponent<RigidBodyComponent>();
		component.BodyType = type;
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

		Ref<PhysicsActor> actor = PhysicsManager::GetScene()->GetActor(entity);
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

		Ref<PhysicsActor> actor = PhysicsManager::GetScene()->GetActor(entity);
		actor->AddTorque(*torque, forceMode);
	}

	void NR_RigidBodyComponent_GetVelocity(uint64_t entityID, glm::vec3* outVelocity)
	{
		Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
		NR_CORE_ASSERT(scene, "No active scene!");
		const auto& entityMap = scene->GetEntityMap();
		NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = entityMap.at(entityID);
		NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
		auto& component = entity.GetComponent<RigidBodyComponent>();

		NR_CORE_ASSERT(outVelocity);
		Ref<PhysicsActor> actor = PhysicsManager::GetScene()->GetActor(entity);
		*outVelocity = actor->GetVelocity();
	}

	void NR_RigidBodyComponent_SetVelocity(uint64_t entityID, glm::vec3* velocity)
	{
		Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
		NR_CORE_ASSERT(scene, "No active scene!");
		const auto& entityMap = scene->GetEntityMap();
		NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = entityMap.at(entityID);
		NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
		auto& component = entity.GetComponent<RigidBodyComponent>();

		NR_CORE_ASSERT(velocity);
		Ref<PhysicsActor> actor = PhysicsManager::GetScene()->GetActor(entity);
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
		Ref<PhysicsActor> actor = PhysicsManager::GetScene()->GetActor(entity);
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
		Ref<PhysicsActor> actor = PhysicsManager::GetScene()->GetActor(entity);
		actor->SetAngularVelocity(*velocity);
	}

	float NR_RigidBodyComponent_GetMaxVelocity(uint64_t entityID)
	{
		Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
		NR_CORE_ASSERT(scene, "No active scene!");
		const auto& entityMap = scene->GetEntityMap();
		NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = entityMap.at(entityID);
		NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

		Ref<PhysicsActor> actor = PhysicsManager::GetScene()->GetActor(entity);
		return actor->GetMaxVelocity();
	}

	void NR_RigidBodyComponent_SetMaxVelocity(uint64_t entityID, float maxVelocity)
	{
		Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
		NR_CORE_ASSERT(scene, "No active scene!");
		const auto& entityMap = scene->GetEntityMap();
		NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = entityMap.at(entityID);
		NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

		Ref<PhysicsActor> actor = PhysicsManager::GetScene()->GetActor(entity);
		actor->SetMaxVelocity(maxVelocity);
	}
	float NR_RigidBodyComponent_GetMaxAngularVelocity(uint64_t entityID)
	{
		Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
		NR_CORE_ASSERT(scene, "No active scene!");
		const auto& entityMap = scene->GetEntityMap();
		NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = entityMap.at(entityID);
		NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

		Ref<PhysicsActor> actor = PhysicsManager::GetScene()->GetActor(entity);
		return actor->GetMaxAngularVelocity();
	}
	void NR_RigidBodyComponent_SetMaxAngularVelocity(uint64_t entityID, float maxVelocity)
	{
		Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
		NR_CORE_ASSERT(scene, "No active scene!");
		const auto& entityMap = scene->GetEntityMap();
		NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = entityMap.at(entityID);
		NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
		Ref<PhysicsActor> actor = PhysicsManager::GetScene()->GetActor(entity);

		actor->SetMaxAngularVelocity(maxVelocity);
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

		const Ref<PhysicsActor>& actor = PhysicsManager::GetScene()->GetActor(entity);
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

		Ref<PhysicsActor> actor = PhysicsManager::GetScene()->GetActor(entity);
		actor->SetMass(mass);
	}

	void NR_RigidBodyComponent_GetKinematicTarget(uint64_t entityID, glm::vec3* outTargetPosition, glm::vec3* outTargetRotation)
	{
		Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
		NR_CORE_ASSERT(scene, "No active scene!");

		const auto& entityMap = scene->GetEntityMap();
		NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = entityMap.at(entityID);
		NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

		auto& component = entity.GetComponent<RigidBodyComponent>();
		Ref<PhysicsActor> actor = PhysicsManager::GetScene()->GetActor(entity);
		*outTargetPosition = actor->GetKinematicTargetPosition();
		*outTargetRotation = actor->GetKinematicTargetRotation();
	}

	void NR_RigidBodyComponent_SetKinematicTarget(uint64_t entityID, glm::vec3* inTargetPosition, glm::vec3* inTargetRotation)
	{
		Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
		NR_CORE_ASSERT(scene, "No active scene!");

		const auto& entityMap = scene->GetEntityMap();
		NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = entityMap.at(entityID);
		NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

		auto& component = entity.GetComponent<RigidBodyComponent>();
		Ref<PhysicsActor> actor = PhysicsManager::GetScene()->GetActor(entity);
		actor->SetKinematicTarget(*inTargetPosition, *inTargetRotation);
	}

	Ref<Mesh>* NR_Mesh_Constructor(MonoString* filepath)
	{
		AssetHandle handle = AssetManager::GetAssetHandleFromFilePath(mono_string_to_utf8(filepath));
		if (AssetManager::IsAssetHandleValid(handle))
		{
			return new Ref<Mesh>(new Mesh(AssetManager::GetAsset<MeshAsset>(handle)));
		}

		return new Ref<Mesh>(new Mesh(Ref<MeshAsset>::Create(mono_string_to_utf8(filepath))));
	}

	void NR_Mesh_Destructor(Ref<Mesh>* _this)
	{
		Ref<Mesh>* instance = (Ref<Mesh>*)_this;
		delete _this;
	}

	Ref<MaterialAsset>* NR_Mesh_GetMaterial(Ref<Mesh>* inMesh)
	{
		Ref<Mesh>& mesh = *(Ref<Mesh>*)inMesh;
		const auto& materialTable = mesh->GetMaterials();
		return new Ref<MaterialAsset>(materialTable->GetMaterial(0));
	}

	Ref<MaterialAsset>* NR_Mesh_GetMaterialByIndex(Ref<Mesh>* inMesh, int index)
	{
		Ref<Mesh>& mesh = *(Ref<Mesh>*)inMesh;
		const auto& materialTable = mesh->GetMaterials();

		NR_CORE_ASSERT(index < materialTable->GetMaterialCount());
		return new Ref<MaterialAsset>(materialTable->GetMaterial(index));
	}

	uint32_t NR_Mesh_GetMaterialCount(Ref<Mesh>* inMesh)
	{
		Ref<Mesh>& mesh = *inMesh;
		const auto& materials = mesh->GetMaterials()->GetMaterials();
		return (uint32_t)materials.size();
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
		for (uint32_t i = 0; i < instance->GetWidth() * instance->GetHeight(); ++i)
		{
			glm::vec4& value = mono_array_get(inData, glm::vec4, i);
			*pixels++ = (uint32_t)(value.x * 255.0f);
			*pixels++ = (uint32_t)(value.y * 255.0f);
			*pixels++ = (uint32_t)(value.z * 255.0f);
			*pixels++ = (uint32_t)(value.w * 255.0f);
		}

		instance->Unlock();
	}

	void NR_Material_Destructor(Ref<MaterialAsset>* _this)
	{
		delete _this;
	}

	void NR_Material_GetAlbedoColor(Ref<MaterialAsset>* _this, glm::vec3* outAlbedoColor)
	{
		Ref<MaterialAsset>& instance = *(Ref<MaterialAsset>*)_this;
		*outAlbedoColor = instance->GetAlbedoColor();
	}

	void NR_Material_SetAlbedoColor(Ref<MaterialAsset>* _this, glm::vec3* inAlbedoColor)
	{
		Ref<MaterialAsset>& instance = *(Ref<MaterialAsset>*)_this;
		instance->SetAlbedoColor(*inAlbedoColor);
	}

	void NR_Material_GetMetalness(Ref<MaterialAsset>* _this, float* outMetalness)
	{
		Ref<MaterialAsset>& instance = *(Ref<MaterialAsset>*)_this;
		*outMetalness = instance->GetMetalness();
	}
	void NR_Material_SetMetalness(Ref<MaterialAsset>* _this, float inMetalness)
	{
		Ref<MaterialAsset>& instance = *(Ref<MaterialAsset>*)_this;
		instance->SetMetalness(inMetalness);
	}

	void NR_Material_GetRoughness(Ref<MaterialAsset>* _this, float* outRoughness)
	{
		Ref<MaterialAsset>& instance = *(Ref<MaterialAsset>*)_this;
		*outRoughness = instance->GetRoughness();
	}

	void NR_Material_SetRoughness(Ref<MaterialAsset>* _this, float inRoughness)
	{
		Ref<MaterialAsset>& instance = *(Ref<MaterialAsset>*)_this;
		instance->SetRoughness(inRoughness);
	}

	void NR_Material_SetFloat(Ref<MaterialAsset>* _this, MonoString* uniform, float value)
	{
		Ref<MaterialAsset>& instance = *(Ref<MaterialAsset>*)_this;
		instance->GetMaterial()->Set(mono_string_to_utf8(uniform), value);
	}

	void NR_Material_SetTexture(Ref<MaterialAsset>* _this, MonoString* uniform, Ref<Texture2D>* texture)
	{
		Ref<MaterialAsset>& instance = *(Ref<MaterialAsset>*)_this;
		instance->GetMaterial()->Set(mono_string_to_utf8(uniform), *texture);
	}

	void* NR_MeshFactory_CreatePlane(float width, float height)
	{
		return new Ref<Mesh>(new Mesh(Ref<MeshAsset>::Create("Assets/Models/Plane1m.obj")));
	}

	const auto CheckActiveScene = [] { return ScriptEngine::GetCurrentSceneContext(); };

	static inline Entity GetEntityFromScene(uint64_t entityID)
	{
		Ref<Scene> scene = CheckActiveScene();
		NR_CORE_ASSERT(scene, "No active scene!");

		const auto& entityMap = scene->GetEntityMap();
		NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		return entityMap.at(entityID);
	};
	template<typename C >
	static inline C& GetEntityComponent(uint64_t entityID)
	{
		Entity entity = GetEntityFromScene(entityID);
		NR_CORE_ASSERT(entity.HasComponent<C>());
		return entity.GetComponent<C>();
	}

	bool NR_AudioComponent_IsPlaying(uint64_t entityID)
	{
		Entity entity = GetEntityFromScene(entityID);
		NR_CORE_ASSERT(entity.HasComponent<Audio::AudioComponent>());
		return Audio::AudioPlayback::IsPlaying(entityID);
	}

	bool NR_AudioComponent_Play(uint64_t entityID, float startTime)
	{
		Entity entity = GetEntityFromScene(entityID);
		NR_CORE_ASSERT(entity.HasComponent<Audio::AudioComponent>());
		return Audio::AudioPlayback::Play(entityID, startTime);
	}

	bool NR_AudioComponent_Stop(uint64_t entityID)
	{
		Entity entity = GetEntityFromScene(entityID);
		NR_CORE_ASSERT(entity.HasComponent<Audio::AudioComponent>());
		return Audio::AudioPlayback::StopActiveSound(entityID);
	}

	bool NR_AudioComponent_Pause(uint64_t entityID)
	{
		Entity entity = GetEntityFromScene(entityID);
		NR_CORE_ASSERT(entity.HasComponent<Audio::AudioComponent>());
		return Audio::AudioPlayback::PauseActiveSound(entityID);
	}

	float NR_AudioComponent_GetVolumeMult(uint64_t entityID)
	{
		return GetEntityComponent<Audio::AudioComponent>(entityID).VolumeMultiplier;
	}

	void NR_AudioComponent_SetVolumeMult(uint64_t entityID, float volumeMult)
	{
		GetEntityComponent<Audio::AudioComponent>(entityID).VolumeMultiplier = volumeMult;
	}

	float NR_AudioComponent_GetPitchMult(uint64_t entityID)
	{
		return GetEntityComponent<Audio::AudioComponent>(entityID).PitchMultiplier;
	}

	void NR_AudioComponent_SetPitchMult(uint64_t entityID, float pitchMult)
	{
		GetEntityComponent<Audio::AudioComponent>(entityID).PitchMultiplier = pitchMult;
	}

	bool NR_AudioComponent_GetLooping(uint64_t entityID)
	{
		return GetEntityComponent<Audio::AudioComponent>(entityID).SoundConfig->Looping;
	}

	void NR_AudioComponent_SetLooping(uint64_t entityID, bool looping)
	{
		GetEntityComponent<Audio::AudioComponent>(entityID).SoundConfig->Looping = looping;
	}

	float NR_AudioComponent_GetMasterReverbSend(uint64_t entityID)
	{
		return Audio::AudioPlayback::GetMasterReverbSend(entityID);
	}

	void NR_AudioComponent_SetMasterReverbSend(uint64_t entityID, float sendLevel)
	{
		Audio::AudioPlayback::SetMasterReverbSend(entityID, sendLevel);
	}

	void NR_AudioComponent_SetSound(uint64_t entityID, Ref<AudioFile>* sound)
	{
		NR_CORE_ASSERT(CheckActiveScene(), "No active scene!");
		GetEntityComponent<Audio::AudioComponent>(entityID).SoundConfig->FileAsset = *sound;
	}

	void NR_AudioComponent_SetSoundPath(uint64_t entityID, MonoString* filepath)
	{
		auto asset = AssetManager::GetAsset<AudioFile>(mono_string_to_utf8(filepath));
		NR_CORE_ASSERT(asset, "Asset by supplied filepath does not exist!");
		GetEntityComponent<Audio::AudioComponent>(entityID).SoundConfig->FileAsset = asset;
	}

	MonoString* NR_AudioComponent_GetSound(uint64_t entityID)
	{
		auto& audioComponent = GetEntityComponent<Audio::AudioComponent>(entityID);
		const std::string& filepath = AssetManager::GetMetadata(audioComponent.SoundConfig->FileAsset->Handle).FilePath.string();
		return mono_string_new_wrapper(filepath.c_str());
	}

	bool NR_Audio_PlaySound2DAsset(Ref<AudioFile>* sound, float volume, float pitch)
	{
		NR_CORE_ASSERT(CheckActiveScene(), "No active scene!");
		return Audio::AudioPlayback::PlaySound2D(*sound, volume, pitch);
	}

	bool NR_Audio_PlaySound2DAssetPath(MonoString* filepath, float volume, float pitch)
	{
		NR_CORE_ASSERT(CheckActiveScene(), "No active scene!");
		auto asset = AssetManager::GetAsset<AudioFile>(mono_string_to_utf8(filepath));
		NR_CORE_ASSERT(asset, "Asset by supplied filepath does not exist!");
		return Audio::AudioPlayback::PlaySound2D(asset, volume, pitch);
	}

	bool NR_Audio_PlaySoundAtLocationAsset(Ref<AudioFile>* sound, glm::vec3* location, float volume, float pitch)
	{
		NR_CORE_ASSERT(CheckActiveScene(), "No active scene!");
		return Audio::AudioPlayback::PlaySoundAtLocation(*sound, *location, volume, pitch);
	}

	bool NR_Audio_PlaySoundAtLocationAssetPath(MonoString* filepath, glm::vec3* location, float volume, float pitch)
	{
		NR_CORE_ASSERT(CheckActiveScene(), "No active scene!");
		auto asset = AssetManager::GetAsset<AudioFile>(mono_string_to_utf8(filepath));
		NR_CORE_ASSERT(asset, "Asset by supplied filepath does not exist!");
		return Audio::AudioPlayback::PlaySoundAtLocation(asset, *location, volume, pitch);
	}

	Ref<AudioFile>* NR_SimpleSound_Constructor(MonoString* filepath)
	{
		auto asset = AssetManager::GetAsset<AudioFile>(mono_string_to_utf8(filepath));
		NR_CORE_ASSERT(asset, "Asset by supplied filepath does not exist!");
		return new Ref<AudioFile>(AssetManager::GetAsset<AudioFile>(mono_string_to_utf8(filepath)));
	}

	void NR_SimpleSound_Destructor(Ref<AudioFile>* _this)
	{
		delete _this;
	}

	uint64_t NR_AudioCreateSound2DAsset(Ref<AudioFile>* sound, float volume, float pitch)
	{
		auto scene = CheckActiveScene();
		Entity entity = scene->CreateEntityWithID(NR::UUID(), "Sound3D");
		auto& audioComponent = entity.AddComponent<Audio::AudioComponent>();

		audioComponent.SoundConfig->FileAsset = *sound;
		audioComponent.VolumeMultiplier = volume;
		audioComponent.PitchMultiplier = pitch;

		return entity.GetID();
	}

	uint64_t NR_AudioCreateSound2DPath(MonoString* filepath, float volume, float pitch)
	{
		auto asset = AssetManager::GetAsset<AudioFile>(mono_string_to_utf8(filepath));
		NR_CORE_ASSERT(asset, "Asset by supplied filepath does not exist!");
		auto scene = CheckActiveScene();
		Entity entity = scene->CreateEntityWithID(NR::UUID(), "Sound3D");
		auto& audioComponent = entity.AddComponent<Audio::AudioComponent>();

		audioComponent.SoundConfig->FileAsset = asset;
		audioComponent.VolumeMultiplier = volume;
		audioComponent.PitchMultiplier = pitch;

		return entity.GetID();
	}

	uint64_t NR_AudioCreateSound3DAsset(Ref<AudioFile>* sound, glm::vec3* location, float volume, float pitch)
	{
		auto scene = CheckActiveScene();
		Entity entity = scene->CreateEntityWithID(NR::UUID(), "Sound3D");
		auto& audioComponent = entity.AddComponent<Audio::AudioComponent>();

		audioComponent.SoundConfig->FileAsset = *sound;
		audioComponent.SoundConfig->SpatializationEnabled = true;
		audioComponent.SoundConfig->SpawnLocation = *location;
		audioComponent.VolumeMultiplier = volume;
		audioComponent.PitchMultiplier = pitch;
		audioComponent.SourcePosition = *location;

		return entity.GetID();
	}

	uint64_t NR_AudioCreateSound3DPath(MonoString* filepath, glm::vec3* location, float volume, float pitch)
	{
		auto asset = AssetManager::GetAsset<AudioFile>(mono_string_to_utf8(filepath));
		NR_CORE_ASSERT(asset, "Asset by supplied filepath does not exist!");
		auto scene = CheckActiveScene();
		Entity entity = scene->CreateEntityWithID(NR::UUID(), "Sound3D");
		auto& audioComponent = entity.AddComponent<Audio::AudioComponent>();

		audioComponent.SoundConfig->FileAsset = asset;
		audioComponent.SoundConfig->SpatializationEnabled = true;
		audioComponent.SoundConfig->SpawnLocation = *location;
		audioComponent.VolumeMultiplier = volume;
		audioComponent.PitchMultiplier = pitch;
		audioComponent.SourcePosition = *location;

		return entity.GetID();
	}

	void NR_Log_LogMessage(LogLevel level, MonoString* message)
	{
		char* msg = mono_string_to_utf8(message);
		switch (level)
		{
		case LogLevel::Trace:         NR_CONSOLE_LOG_TRACE(msg); break;
		case LogLevel::Debug:         NR_CONSOLE_LOG_INFO(msg); break;
		case LogLevel::Info:          NR_CONSOLE_LOG_INFO(msg); break;
		case LogLevel::Warn:          NR_CONSOLE_LOG_WARN(msg); break;
		case LogLevel::Error:         NR_CONSOLE_LOG_ERROR(msg); break;
		case LogLevel::Critical:      NR_CONSOLE_LOG_FATAL(msg); break;
		}
	}
}