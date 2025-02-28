#include "nrpch.h"
#include "ScriptWrappers.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/common.hpp>

#include <mono/jit/jit.h>

#include <box2d/box2d.h>

#include <PhysX/include/PxPhysicsAPI.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "NotRed/Math/Noise.h"

#include "NotRed/Asset/AssetManager.h"
#include "NotRed/Scene/Scene.h"
#include "NotRed/Scene/Entity.h"
#include "NotRed/Scene/Prefab.h"
#include "NotRed/Audio/AudioComponent.h"
#include "NotRed/Audio/AudioPlayback.h"
#include "NotRed/Audio/AudioEngine.h"
#include "NotRed/Audio/AudioEvents/CommandID.h"
#include "NotRed/Audio/AudioEvents/AudioCommandRegistry.h"
#include "NotRed/Core/Application.h"
#include "NotRed/Script/ScriptEngine.h"
#include "NotRed/Physics/2D/Physics2D.h"

#include "NotRed/Debug/Profiler.h"

namespace NR {
	extern std::unordered_map<MonoType*, std::function<bool(Entity&)>> sHasComponentFuncs;
	extern std::unordered_map<MonoType*, std::function<void(Entity&)>> sCreateComponentFuncs;
}

namespace NR::Script 
{
		static inline auto GetEntity(uint64_t entityID)
		{
			Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
			NR_CORE_ASSERT(scene, "No active scene!");
			const auto& entityMap = scene->GetEntityMap();
			NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

			return entityMap.at(entityID);
		};

		////////////////////////////////////////////////////////////////
		// Math ////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////

		float NR_Noise_PerlinNoise(float x, float y)
		{
			return Noise::PerlinNoise(x, y);
		}

		////////////////////////////////////////////////////////////////

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
					wasPressed = GImGui->HoveredWindow->ID == viewportWindow->ID;
			}

			return wasPressed;
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

		bool NR_Input_IsControllerPresent(int id)
		{
			return Input::IsControllerPresent(id);
		}

		MonoArray* NR_Input_GetConnectedControllerIDs()
		{
			std::vector<int> ids = Input::GetConnectedControllerIDs();

			MonoArray* result = mono_array_new(mono_domain_get(), mono_get_int32_class(), ids.size());
			for (size_t i = 0; i < ids.size(); i++)
				mono_array_set(result, int, i, ids[i]);

			return result;
		}

		MonoString* NR_Input_GetControllerName(int id)
		{
			auto name = Input::GetControllerName(id);
			if (name.empty())
				return mono_string_new(mono_domain_get(), "");

			return mono_string_new(mono_domain_get(), &name.front());
		}

		bool NR_Input_IsControllerButtonPressed(int id, int button)
		{
			return Input::IsControllerButtonPressed(id, button);
		}

		float NR_Input_GetControllerAxis(int id, int axis)
		{
			return Input::GetControllerAxis(id, axis);
		}

		uint8_t NR_Input_GetControllerHat(int id, int hat)
		{
			return Input::GetControllerHat(id, hat);
		}

		MonoArray* NR_Scene_GetEntities()
		{
			Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
			NR_CORE_ASSERT(scene, "No active scene!");
			const auto& entityMap = scene->GetEntityMap();

			MonoArray* result = mono_array_new(mono_domain_get(), ScriptEngine::GetCoreClass("NR.Entity"), entityMap.size());

			uint32_t index = 0;
			for (auto& [id, entity] : entityMap)
			{
				UUID uuid = id;
				void* data[] = { &uuid };
				MonoObject* obj = ScriptEngine::Construct("NR.Entity:.ctor(ulong)", true, data);
				mono_array_set(result, MonoObject*, index++, obj);
			}

			return result;
		}

		bool NR_Physics_Raycast(RaycastData* inData, RaycastHit* hit)
		{
			Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
			NR_CORE_ASSERT(scene, "No active scene!");
			NR_CORE_ASSERT(scene->GetPhysicsScene()->IsValid());

			RaycastHit temp;
			bool success = scene->GetPhysicsScene()->Raycast(inData->Origin, inData->Direction, inData->MaxDistance, &temp);

			if (success && inData->RequiredComponentTypes != nullptr)
			{
				Entity entity = scene->FindEntityByUUID(temp.HitEntity);
				size_t length = mono_array_length(inData->RequiredComponentTypes);

				for (size_t i = 0; i < length; i++)
				{
					void* rawType = mono_array_get(inData->RequiredComponentTypes, void*, i);
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

		MonoArray* NR_Physics_Raycast2D(RaycastData2D* inData)
		{
			Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
			NR_CORE_ASSERT(scene, "No active scene!");

			std::vector<Raycast2DResult> raycastResults = Physics2D::Raycast(scene, inData->Origin, inData->Origin + inData->Direction * inData->MaxDistance);

			MonoArray* results = mono_array_new(mono_domain_get(), ScriptEngine::GetCoreClass("NR.RaycastHit2D"), raycastResults.size());
			for (size_t i = 0; i < raycastResults.size(); i++)
			{
				UUID entityID = raycastResults[i].HitEntity.GetID();
				void* data[] = {
					&entityID
				};

				MonoObject* entity = ScriptEngine::Construct("NR.Entity:.ctor(ulong)", true, data);

				void* rcData[] = {
					entity,
					&raycastResults[i].Point,
					&raycastResults[i].Normal,
					&raycastResults[i].Distance
				};

				MonoObject* obj = ScriptEngine::Construct("NR.RaycastHit2D:.ctor(Entity,Vector2,Vector2,single)", true, rcData);
				mono_array_set(results, MonoObject*, i, obj);
			}

			return results;
		}

		// Helper function for the Overlap functions below
		static void AddCollidersToArray(MonoArray* array, const std::array<OverlapHit, OVERLAP_MAX_COLLIDERS>& hits, uint32_t count, uint32_t arrayLength)
		{
			uint32_t arrayIndex = 0;
			for (uint32_t i = 0; i < count; i++)
			{
				Entity entity = hits[i].Actor->GetEntity();

				if (entity.HasComponent<BoxColliderComponent>() && arrayIndex < arrayLength)
				{
					auto& boxCollider = entity.GetComponent<BoxColliderComponent>();

					void* data[] = {
						&entity.GetID(),
						&boxCollider.IsTrigger,
						&boxCollider.Size,
						&boxCollider.Offset
					};

					MonoObject* obj = ScriptEngine::Construct("NR.BoxCollider:.ctor(ulong,bool,Vector3,Vector3)", true, data);
					mono_array_set(array, MonoObject*, arrayIndex++, obj);
				}

				if (entity.HasComponent<SphereColliderComponent>() && arrayIndex < arrayLength)
				{
					auto& sphereCollider = entity.GetComponent<SphereColliderComponent>();

					void* data[] = {
						&entity.GetID(),
						&sphereCollider.IsTrigger,
						&sphereCollider.Radius
					};

					MonoObject* obj = ScriptEngine::Construct("NR.SphereCollider:.ctor(ulong,bool,single)", true, data);
					mono_array_set(array, MonoObject*, arrayIndex++, obj);
				}

				if (entity.HasComponent<CapsuleColliderComponent>() && arrayIndex < arrayLength)
				{
					auto& capsuleCollider = entity.GetComponent<CapsuleColliderComponent>();

					void* data[] = {
						&entity.GetID(),
						&capsuleCollider.IsTrigger,
						&capsuleCollider.Radius,
						&capsuleCollider.Height
					};

					MonoObject* obj = ScriptEngine::Construct("NR.CapsuleCollider:.ctor(ulong,bool,single,single)", true, data);
					mono_array_set(array, MonoObject*, arrayIndex++, obj);
				}

				if (entity.HasComponent<MeshColliderComponent>() && arrayIndex < arrayLength)
				{
					auto& meshCollider = entity.GetComponent<MeshColliderComponent>();

					Ref<Mesh>* mesh = new Ref<Mesh>(AssetManager::GetAsset<Mesh>(meshCollider.CollisionMesh));
					void* data[] = {
						&entity.GetID(),
						&meshCollider.IsTrigger,
						&mesh
					};

					MonoObject* obj = ScriptEngine::Construct("NR.MeshCollider:.ctor(ulong,bool,intptr)", true, data);
					mono_array_set(array, MonoObject*, arrayIndex++, obj);
				}
			}
		}

		static std::array<OverlapHit, OVERLAP_MAX_COLLIDERS> s_OverlapBuffer;

		MonoArray* NR_Physics_OverlapBox(glm::vec3* origin, glm::vec3* halfSize)
		{
			NR_PROFILE_FUNC();
			Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
			NR_CORE_ASSERT(scene, "No active scene!");
			NR_CORE_ASSERT(scene->GetPhysicsScene()->IsValid());

			MonoArray* outColliders = nullptr;
			memset(s_OverlapBuffer.data(), 0, OVERLAP_MAX_COLLIDERS * sizeof(OverlapHit));

			uint32_t count;
			if (scene->GetPhysicsScene()->OverlapBox(*origin, *halfSize, s_OverlapBuffer, count))
			{
				outColliders = mono_array_new(mono_domain_get(), ScriptEngine::GetCoreClass("NR.Collider"), count);
				AddCollidersToArray(outColliders, s_OverlapBuffer, count, count);
			}

			return outColliders;
		}

		MonoArray* NR_Physics_OverlapCapsule(glm::vec3* origin, float radius, float halfHeight)
		{
			NR_PROFILE_FUNC();
			Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
			NR_CORE_ASSERT(scene, "No active scene!");

			MonoArray* outColliders = nullptr;
			memset(s_OverlapBuffer.data(), 0, OVERLAP_MAX_COLLIDERS * sizeof(OverlapHit));

			uint32_t count;
			if (scene->GetPhysicsScene()->OverlapCapsule(*origin, radius, halfHeight, s_OverlapBuffer, count))
			{
				outColliders = mono_array_new(mono_domain_get(), ScriptEngine::GetCoreClass("NR.Collider"), count);
				AddCollidersToArray(outColliders, s_OverlapBuffer, count, count);
			}

			return outColliders;
		}

		MonoArray* NR_Physics_OverlapSphere(glm::vec3* origin, float radius)
		{
			NR_PROFILE_FUNC();
			Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
			NR_CORE_ASSERT(scene, "No active scene!");

			MonoArray* outColliders = nullptr;
			memset(s_OverlapBuffer.data(), 0, OVERLAP_MAX_COLLIDERS * sizeof(OverlapHit));

			uint32_t count;
			if (scene->GetPhysicsScene()->OverlapSphere(*origin, radius, s_OverlapBuffer, count))
			{
				outColliders = mono_array_new(mono_domain_get(), ScriptEngine::GetCoreClass("NR.Collider"), count);
				AddCollidersToArray(outColliders, s_OverlapBuffer, count, count);
			}

			return outColliders;
		}

		int32_t NR_Physics_OverlapBoxNonAlloc(glm::vec3* origin, glm::vec3* halfSize, MonoArray* outColliders)
		{
			NR_PROFILE_FUNC();
			Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
			NR_CORE_ASSERT(scene, "No active scene!");

			memset(s_OverlapBuffer.data(), 0, OVERLAP_MAX_COLLIDERS * sizeof(OverlapHit));

			const uint32_t arrayLength = (uint32_t)mono_array_length(outColliders);

			uint32_t count;
			if (scene->GetPhysicsScene()->OverlapBox(*origin, *halfSize, s_OverlapBuffer, count))
			{
				if (count > arrayLength)
					count = arrayLength;

				AddCollidersToArray(outColliders, s_OverlapBuffer, count, arrayLength);
			}

			return count;
		}

		int32_t NR_Physics_OverlapCapsuleNonAlloc(glm::vec3* origin, float radius, float halfHeight, MonoArray* outColliders)
		{
			NR_PROFILE_FUNC();
			Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
			NR_CORE_ASSERT(scene, "No active scene!");

			memset(s_OverlapBuffer.data(), 0, OVERLAP_MAX_COLLIDERS * sizeof(OverlapHit));

			const uint32_t arrayLength = (uint32_t)mono_array_length(outColliders);
			uint32_t count;
			if (scene->GetPhysicsScene()->OverlapCapsule(*origin, radius, halfHeight, s_OverlapBuffer, count))
			{
				if (count > arrayLength)
					count = arrayLength;

				AddCollidersToArray(outColliders, s_OverlapBuffer, count, arrayLength);
			}

			return count;
		}

		int32_t NR_Physics_OverlapSphereNonAlloc(glm::vec3* origin, float radius, MonoArray* outColliders)
		{
			NR_PROFILE_FUNC();
			Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
			NR_CORE_ASSERT(scene, "No active scene!");

			memset(s_OverlapBuffer.data(), 0, OVERLAP_MAX_COLLIDERS * sizeof(OverlapHit));

			const uint32_t arrayLength = (uint32_t)mono_array_length(outColliders);

			uint32_t count;
			if (scene->GetPhysicsScene()->OverlapSphere(*origin, radius, s_OverlapBuffer, count))
			{
				if (count > arrayLength)
					count = arrayLength;

				AddCollidersToArray(outColliders, s_OverlapBuffer, count, arrayLength);
			}

			return count;
		}

		void NR_Physics_GetGravity(glm::vec3* outGravity)
		{
			Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
			NR_CORE_ASSERT(scene, "No active scene!");
			*outGravity = scene->GetPhysicsScene()->GetGravity();
		}

		void NR_Physics_SetGravity(glm::vec3* inGravity)
		{
			Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
			NR_CORE_ASSERT(scene, "No active scene!");
			scene->GetPhysicsScene()->SetGravity(*inGravity);
		}


		void NR_Physics_AddRadialImpulse(glm::vec3* inOrigin, float radius, float strength, EFalloffMode falloff, bool velocityChange)
		{
			Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
			NR_CORE_ASSERT(scene, "No active scene!");
			scene->GetPhysicsScene()->AddRadialImpulse(*inOrigin, radius, strength, falloff, velocityChange);
		}

		////////////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////////////
		// Entity //////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////

		uint64_t NR_Entity_GetParent(uint64_t entityID)
		{
			return GetEntity(entityID).GetParentUUID();
		}

		void NR_Entity_SetParent(uint64_t entityID, uint64_t parentID)
		{
			auto child = GetEntity(entityID);
			auto parent = GetEntity(parentID);
			child.SetParent(parent);
		}

		MonoArray* NR_Entity_GetChildren(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
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

		uint64_t NR_Entity_CreateEntity()
		{
			Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
			return scene->CreateEntity("Unnamed from C#").GetID();
		}

		uint64_t NR_Entity_Instantiate(uint64_t prefabID)
		{
			Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();

			if (!AssetManager::IsAssetHandleValid(prefabID))
				return 0;

			Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(prefabID);
			return scene->Instantiate(prefab).GetID();
		}

		uint64_t NR_Entity_InstantiateWithTranslation(uint64_t prefabID, glm::vec3* inTranslation)
		{
			Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();

			if (!AssetManager::IsAssetHandleValid(prefabID))
				return 0;

			Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(prefabID);
			return scene->Instantiate(prefab, inTranslation).GetID();
		}

		void NR_Entity_DestroyEntity(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
			scene->SubmitToDestroyEntity(entity);
		}

		void NR_Entity_CreateComponent(uint64_t entityID, void* type)
		{
			auto entity = GetEntity(entityID);
			MonoType* monoType = mono_reflection_type_get_type((MonoReflectionType*)type);
			sCreateComponentFuncs[monoType](entity);
		}

		bool NR_Entity_HasComponent(uint64_t entityID, void* type)
		{
			auto entity = GetEntity(entityID);
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
				return entity.GetComponent<IDComponent>().ID;

			return 0;
		}

		MonoString* NR_TagComponent_GetTag(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			auto& tagComponent = entity.GetComponent<TagComponent>();
			return mono_string_new(mono_domain_get(), tagComponent.Tag.c_str());
		}

		void NR_TagComponent_SetTag(uint64_t entityID, MonoString* tag)
		{
			auto entity = GetEntity(entityID);
			auto& tagComponent = entity.GetComponent<TagComponent>();
			tagComponent.Tag = mono_string_to_utf8(tag);
		}

		void NR_TransformComponent_GetTransform(uint64_t entityID, TransformComponent* outTransform)
		{
			auto entity = GetEntity(entityID);
			*outTransform = entity.GetComponent<TransformComponent>();
		}

		void NR_TransformComponent_SetTransform(uint64_t entityID, TransformComponent* inTransform)
		{
			auto entity = GetEntity(entityID);
			entity.GetComponent<TransformComponent>() = *inTransform;
		}

		void NR_TransformComponent_GetTranslation(uint64_t entityID, glm::vec3* outTranslation)
		{
			auto entity = GetEntity(entityID);
			*outTranslation = entity.GetComponent<TransformComponent>().Translation;
		}

		void NR_TransformComponent_SetTranslation(uint64_t entityID, glm::vec3* inTranslation)
		{
			auto entity = GetEntity(entityID);
			entity.GetComponent<TransformComponent>().Translation = *inTranslation;

			if (entity.HasComponent<RigidBodyComponent>())
			{
				auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
				NR_CORE_ASSERT(actor);
				actor->SetTranslation(*inTranslation);
			}
		}

		void NR_TransformComponent_GetRotation(uint64_t entityID, glm::vec3* outRotation)
		{
			auto entity = GetEntity(entityID);
			*outRotation = entity.GetComponent<TransformComponent>().Rotation;
		}

		void NR_TransformComponent_SetRotation(uint64_t entityID, glm::vec3* inRotation)
		{
			auto entity = GetEntity(entityID);
			entity.GetComponent<TransformComponent>().Rotation = *inRotation;

			if (entity.HasComponent<RigidBodyComponent>())
			{
				auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
				NR_CORE_ASSERT(actor);
				actor->SetRotation(*inRotation);
			}
		}

		void NR_TransformComponent_GetScale(uint64_t entityID, glm::vec3* outScale)
		{
			auto entity = GetEntity(entityID);
			*outScale = entity.GetComponent<TransformComponent>().Scale;
		}

		void NR_TransformComponent_SetScale(uint64_t entityID, glm::vec3* inScale)
		{
			auto entity = GetEntity(entityID);
			entity.GetComponent<TransformComponent>().Scale = *inScale;
		}

		void NR_TransformComponent_GetWorldSpaceTransform(uint64_t entityID, TransformComponent* outTransform)
		{
			auto entity = GetEntity(entityID);
			Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
			*outTransform = scene->GetWorldSpaceTransform(entity);
		}

		void* NR_MeshComponent_GetMesh(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			auto& meshComponent = entity.GetComponent<MeshComponent>();
			auto mesh = AssetManager::GetAsset<Mesh>(meshComponent.Mesh);
			return new Ref<Mesh>(mesh);
		}

		void NR_MeshComponent_SetMesh(uint64_t entityID, Ref<Mesh>* inMesh)
		{
			auto entity = GetEntity(entityID);
			auto& meshComponent = entity.GetComponent<MeshComponent>();
			meshComponent.Mesh = inMesh ? (*inMesh)->Handle : 0;
		}

		bool NR_MeshComponent_HasMaterial(uint64_t entityID, int index)
		{
			auto entity = GetEntity(entityID);
			auto& meshComponent = entity.GetComponent<MeshComponent>();
			const auto& materialTable = meshComponent.MaterialTable;
			return materialTable->HasMaterial(index);
		}

		Ref<MaterialAsset>* NR_MeshComponent_GetMaterial(uint64_t entityID, int index)
		{
			auto entity = GetEntity(entityID);
			auto& meshComponent = entity.GetComponent<MeshComponent>();
			const auto& materialTable = meshComponent.MaterialTable;
			return new Ref<MaterialAsset>(materialTable->GetMaterial(index));
		}

		MonoObject* NR_ScriptComponent_GetInstance(uint64_t entityID)
		{
			Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
			NR_CORE_ASSERT(scene, "No active scene!");
			return ScriptEngine::GetEntityInstanceData(scene->GetID(), entityID).Instance.GetInstance();
		}

		void NR_RigidBody2DComponent_GetBodyType(uint64_t entityID, RigidBody2DComponent::Type* outType)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBody2DComponent>());
			auto& component = entity.GetComponent<RigidBody2DComponent>();
			*outType = component.BodyType;
		}

		void NR_RigidBody2DComponent_SetBodyType(uint64_t entityID, RigidBody2DComponent::Type* type)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBody2DComponent>());
			auto& component = entity.GetComponent<RigidBody2DComponent>();
			component.BodyType = *type;
		}

		void NR_RigidBody2DComponent_GetTranslation(uint64_t entityID, glm::vec2* outTranslation)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBody2DComponent>());
			auto& component = entity.GetComponent<RigidBody2DComponent>();
			b2Body* body = (b2Body*)component.RuntimeBody;
			const b2Vec2& translation = body->GetPosition();
			outTranslation->x = translation.x;
			outTranslation->y = translation.y;
		}

		void NR_RigidBody2DComponent_SetTranslation(uint64_t entityID, glm::vec2* inTranslation)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBody2DComponent>());
			auto& component = entity.GetComponent<RigidBody2DComponent>();
			b2Body* body = (b2Body*)component.RuntimeBody;
			body->SetTransform(b2Vec2(inTranslation->x, inTranslation->y), body->GetAngle());
		}

		void NR_RigidBody2DComponent_GetRotation(uint64_t entityID, float* outRotation)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBody2DComponent>());
			auto& component = entity.GetComponent<RigidBody2DComponent>();
			b2Body* body = (b2Body*)component.RuntimeBody;
			NR_CORE_ASSERT(body->GetType() == b2_dynamicBody, "Trying to add force to non dynamic body");
			*outRotation = body->GetAngle();
		}

		void NR_RigidBody2DComponent_SetRotation(uint64_t entityID, float* inRotation)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBody2DComponent>());
			auto& component = entity.GetComponent<RigidBody2DComponent>();
			b2Body* body = (b2Body*)component.RuntimeBody;
			NR_CORE_ASSERT(body->GetType() == b2_dynamicBody, "Trying to add force to non dynamic body");
			body->SetTransform(body->GetTransform().p, *inRotation);
		}

		void NR_RigidBody2DComponent_AddForce(uint64_t entityID, glm::vec2* force, glm::vec2* offset, bool wake)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBody2DComponent>());
			auto& component = entity.GetComponent<RigidBody2DComponent>();
			b2Body* body = (b2Body*)component.RuntimeBody;
			NR_CORE_ASSERT(body->GetType() == b2_dynamicBody, "Trying to add force to non dynamic body");
			body->ApplyForce(*(const b2Vec2*)force, body->GetWorldCenter() + *(const b2Vec2*)offset, wake);
		}

		void NR_RigidBody2DComponent_AddTorque(uint64_t entityID, float* torque, bool wake)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBody2DComponent>());
			auto& component = entity.GetComponent<RigidBody2DComponent>();
			b2Body* body = (b2Body*)component.RuntimeBody;
			NR_CORE_ASSERT(body->GetType() == b2_dynamicBody, "Trying to add force to non dynamic body");
			body->ApplyTorque(*torque, wake);
		}

		void NR_RigidBody2DComponent_ApplyLinearImpulse(uint64_t entityID, glm::vec2* impulse, glm::vec2* offset, bool wake)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBody2DComponent>());
			auto& component = entity.GetComponent<RigidBody2DComponent>();
			b2Body* body = (b2Body*)component.RuntimeBody;
			NR_CORE_ASSERT(body->GetType() == b2_dynamicBody, "Trying to add force to non dynamic body");
			body->ApplyLinearImpulse(*(const b2Vec2*)impulse, body->GetWorldCenter() + *(const b2Vec2*)offset, wake);
		}

		void NR_RigidBody2DComponent_ApplyAngularImpulse(uint64_t entityID, float* impulse, bool wake)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBody2DComponent>());
			auto& component = entity.GetComponent<RigidBody2DComponent>();
			b2Body* body = (b2Body*)component.RuntimeBody;
			NR_CORE_ASSERT(body->GetType() == b2_dynamicBody, "Trying to add force to non dynamic body");
			body->ApplyAngularImpulse(*impulse, wake);
		}

		void NR_RigidBody2DComponent_GetLinearVelocity(uint64_t entityID, glm::vec2* outVelocity)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBody2DComponent>());
			auto& component = entity.GetComponent<RigidBody2DComponent>();
			b2Body* body = (b2Body*)component.RuntimeBody;
			const auto& velocity = body->GetLinearVelocity();
			*outVelocity = { velocity.x, velocity.y };
		}

		void NR_RigidBody2DComponent_SetLinearVelocity(uint64_t entityID, glm::vec2* inVelocity)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBody2DComponent>());
			auto& component = entity.GetComponent<RigidBody2DComponent>();
			b2Body* body = (b2Body*)component.RuntimeBody;
			NR_CORE_ASSERT(body->GetType() == b2_dynamicBody, "Trying to add force to non dynamic body");
			body->SetLinearVelocity({ inVelocity->x, inVelocity->y });
		}

		void NR_RigidBody2DComponent_GetGravityScale(uint64_t entityID, float* outGravityScale)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBody2DComponent>());
			auto& component = entity.GetComponent<RigidBody2DComponent>();
			b2Body* body = (b2Body*)component.RuntimeBody;
			*outGravityScale = body->GetGravityScale();
		}

		void NR_RigidBody2DComponent_SetGravityScale(uint64_t entityID, float* inGravityScale)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBody2DComponent>());
			auto& component = entity.GetComponent<RigidBody2DComponent>();
			b2Body* body = (b2Body*)component.RuntimeBody;
			body->SetGravityScale(*inGravityScale);
		}

		void NR_RigidBody2DComponent_GetMass(uint64_t entityID, float* outMass)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBody2DComponent>());
			auto& component = entity.GetComponent<RigidBody2DComponent>();
			b2Body* body = (b2Body*)component.RuntimeBody;
			//outMass* = body->GetMass();
		}

		void NR_RigidBody2DComponent_SetMass(uint64_t entityID, float* inMass)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBody2DComponent>());
			auto& component = entity.GetComponent<RigidBody2DComponent>();
			b2Body* body = (b2Body*)component.RuntimeBody;
			//body.Mass = &inMass;
		}

		RigidBodyComponent::Type NR_RigidBodyComponent_GetBodyType(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
			auto& component = entity.GetComponent<RigidBodyComponent>();
			return component.BodyType;
		}

		void NR_RigidBodyComponent_SetBodyType(uint64_t entityID, RigidBodyComponent::Type type)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
			auto& component = entity.GetComponent<RigidBodyComponent>();
			component.BodyType = type;

			//Ref<PhysicsActor> actor = scene->GetPhysicsWorld()->GetActor(entity);
			//actor->SetBodyType(type);
		}

		void NR_RigidBodyComponent_GetTranslation(uint64_t entityID, glm::vec3* outTranslation)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			*outTranslation = actor->GetTranslation();
		}

		void NR_RigidBodyComponent_SetTranslation(uint64_t entityID, glm::vec3* inTranslation)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
			auto& component = entity.GetComponent<RigidBodyComponent>();

			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			actor->SetTranslation(*inTranslation);
		}

		void NR_RigidBodyComponent_GetRotation(uint64_t entityID, glm::vec3* outRotation)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
			auto& component = entity.GetComponent<RigidBodyComponent>();
			NR_CORE_ASSERT(outRotation);
			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			*outRotation = actor->GetRotation();
		}

		void NR_RigidBodyComponent_SetRotation(uint64_t entityID, glm::vec3* inRotation)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
			auto& component = entity.GetComponent<RigidBodyComponent>();
			NR_CORE_ASSERT(inRotation);
			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			actor->SetRotation(*inRotation);
		}

		void NR_RigidBodyComponent_Rotate(uint64_t entityID, glm::vec3* inRotation)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
			auto& component = entity.GetComponent<RigidBodyComponent>();
			NR_CORE_ASSERT(inRotation);
			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			actor->Rotate(*inRotation);
		}

		void NR_RigidBodyComponent_AddForce(uint64_t entityID, glm::vec3* force, ForceMode forceMode)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
			auto& component = entity.GetComponent<RigidBodyComponent>();

			if (component.IsKinematic || component.BodyType == RigidBodyComponent::Type::Static)
			{
				NR_CONSOLE_LOG_WARN("Cannot add a force to a static or kinematic actor! EntityID({0})", entityID);
				return;
			}

			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			actor->AddForce(*force, forceMode);
		}

		void NR_RigidBodyComponent_AddTorque(uint64_t entityID, glm::vec3* torque, ForceMode forceMode)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
			auto& component = entity.GetComponent<RigidBodyComponent>();

			if (component.IsKinematic || component.BodyType == RigidBodyComponent::Type::Static)
			{
				NR_CONSOLE_LOG_WARN("Cannot add torque to a static or kinematic actor! EntityID({0})", entityID);
				return;
			}

			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			actor->AddTorque(*torque, forceMode);
		}

		void NR_RigidBodyComponent_GetLinearVelocity(uint64_t entityID, glm::vec3* outVelocity)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
			auto& component = entity.GetComponent<RigidBodyComponent>();

			if (component.IsKinematic || component.BodyType == RigidBodyComponent::Type::Static)
			{
				NR_CONSOLE_LOG_WARN("Cannot get linear velocity from a static or kinematic actor! EntityID({0})", entityID);
				return;
			}

			NR_CORE_ASSERT(outVelocity);
			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			*outVelocity = actor->GetLinearVelocity();
		}

		void NR_RigidBodyComponent_SetLinearVelocity(uint64_t entityID, glm::vec3* inVelocity)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
			auto& component = entity.GetComponent<RigidBodyComponent>();

			if (component.IsKinematic || component.BodyType == RigidBodyComponent::Type::Static)
			{
				NR_CONSOLE_LOG_WARN("Cannot set linear velocity of a static or kinematic actor! EntityID({0})", entityID);
				return;
			}

			ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity)->SetLinearVelocity(*inVelocity);
		}

		void NR_RigidBodyComponent_GetAngularVelocity(uint64_t entityID, glm::vec3* outVelocity)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
			auto& component = entity.GetComponent<RigidBodyComponent>();

			if (component.IsKinematic || component.BodyType == RigidBodyComponent::Type::Static)
			{
				NR_CONSOLE_LOG_WARN("Cannot get angular velocity of a static or kinematic actor! EntityID({0})", entityID);
				return;
			}

			NR_CORE_ASSERT(outVelocity);
			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			*outVelocity = actor->GetAngularVelocity();
		}

		void NR_RigidBodyComponent_SetAngularVelocity(uint64_t entityID, glm::vec3* inVelocity)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
			auto& component = entity.GetComponent<RigidBodyComponent>();

			if (component.IsKinematic || component.BodyType == RigidBodyComponent::Type::Static)
			{
				NR_CONSOLE_LOG_WARN("Cannot set angular velocity of a static or kinematic actor! EntityID({0})", entityID);
				return;
			}

			NR_CORE_ASSERT(inVelocity);
			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			actor->SetAngularVelocity(*inVelocity);
		}

		float NR_RigidBodyComponent_GetMaxLinearVelocity(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

			auto& component = entity.GetComponent<RigidBodyComponent>();
			if (component.IsKinematic || component.BodyType == RigidBodyComponent::Type::Static)
			{
				NR_CONSOLE_LOG_WARN("Cannot get max linear velocity of a static or kinematic actor! EntityID({0})", entityID);
				return 0.0f;
			}

			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			return actor->GetMaxLinearVelocity();
		}

		void NR_RigidBodyComponent_SetMaxLinearVelocity(uint64_t entityID, float maxVelocity)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

			auto& component = entity.GetComponent<RigidBodyComponent>();
			if (component.IsKinematic || component.BodyType == RigidBodyComponent::Type::Static)
			{
				NR_CONSOLE_LOG_WARN("Cannot set max linear velocity of a static or kinematic actor! EntityID({0})", entityID);
				return;
			}

			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			actor->SetMaxLinearVelocity(maxVelocity);
		}

		float NR_RigidBodyComponent_GetMaxAngularVelocity(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

			auto& component = entity.GetComponent<RigidBodyComponent>();
			if (component.IsKinematic || component.BodyType == RigidBodyComponent::Type::Static)
			{
				NR_CONSOLE_LOG_WARN("Cannot get max angular velocity of a static or kinematic actor! EntityID({0})", entityID);
				return 0.0f;
			}

			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			return actor->GetMaxAngularVelocity();
		}

		void NR_RigidBodyComponent_SetMaxAngularVelocity(uint64_t entityID, float maxVelocity)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

			auto& component = entity.GetComponent<RigidBodyComponent>();
			if (component.IsKinematic || component.BodyType == RigidBodyComponent::Type::Static)
			{
				NR_CONSOLE_LOG_WARN("Cannot set max angular velocity of a static or kinematic actor! EntityID({0})", entityID);
				return;
			}

			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			actor->SetMaxAngularVelocity(maxVelocity);
		}

		float NR_RigidBodyComponent_GetLinearDrag(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

			auto& component = entity.GetComponent<RigidBodyComponent>();
			if (component.IsKinematic || component.BodyType == RigidBodyComponent::Type::Static)
			{
				NR_CONSOLE_LOG_WARN("Cannot get linear drag of a static or kinematic actor! EntityID({0})", entityID);
				return 0.0f;
			}

			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			return actor->GetLinearDrag();
		}

		void NR_RigidBodyComponent_SetLinearDrag(uint64_t entityID, float linearDrag)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

			auto& component = entity.GetComponent<RigidBodyComponent>();
			if (component.IsKinematic || component.BodyType == RigidBodyComponent::Type::Static)
			{
				NR_CONSOLE_LOG_WARN("Cannot set linear drag of a static or kinematic actor! EntityID({0})", entityID);
				return;
			}

			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			actor->SetLinearDrag(linearDrag);
		}

		float NR_RigidBodyComponent_GetAngularDrag(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

			auto& component = entity.GetComponent<RigidBodyComponent>();
			if (component.IsKinematic || component.BodyType == RigidBodyComponent::Type::Static)
			{
				NR_CONSOLE_LOG_WARN("Cannot get angular drag of a static or kinematic actor! EntityID({0})", entityID);
				return 0.0f;
			}

			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			return actor->GetAngularDrag();
		}

		void NR_RigidBodyComponent_SetAngularDrag(uint64_t entityID, float angularDrag)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

			auto& component = entity.GetComponent<RigidBodyComponent>();
			if (component.IsKinematic || component.BodyType == RigidBodyComponent::Type::Static)
			{
				NR_CONSOLE_LOG_WARN("Cannot set angular drag of a static or kinematic actor! EntityID({0})", entityID);
				return;
			}

			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			actor->SetAngularDrag(angularDrag);
		}

		uint32_t NR_RigidBodyComponent_GetLayer(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
			auto& component = entity.GetComponent<RigidBodyComponent>();
			return component.Layer;
		}

		float NR_RigidBodyComponent_GetMass(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

			auto& component = entity.GetComponent<RigidBodyComponent>();
			if (component.IsKinematic || component.BodyType == RigidBodyComponent::Type::Static)
			{
				NR_CONSOLE_LOG_WARN("Cannot get mass of a static or kinematic actor! EntityID({0})", entityID);
				return 0.0f;
			}

			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			return actor->GetMass();
		}

		void NR_RigidBodyComponent_SetMass(uint64_t entityID, float mass)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

			auto& component = entity.GetComponent<RigidBodyComponent>();
			if (component.IsKinematic || component.BodyType == RigidBodyComponent::Type::Static)
			{
				NR_CONSOLE_LOG_WARN("Cannot set mass of a static or kinematic actor! EntityID({0})", entityID);
				return;
			}

			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			actor->SetMass(mass);
		}

		void NR_RigidBodyComponent_GetKinematicTarget(uint64_t entityID, glm::vec3* outTargetPosition, glm::vec3* outTargetRotation)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

			auto& component = entity.GetComponent<RigidBodyComponent>();
			if (!component.IsKinematic)
			{
				NR_CONSOLE_LOG_WARN("Cannot get kinematic target of a non-kinematic actor! EntityID({0})", entityID);
				return;
			}

			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			*outTargetPosition = actor->GetKinematicTargetPosition();
			*outTargetRotation = actor->GetKinematicTargetRotation();
		}

		void NR_RigidBodyComponent_SetKinematicTarget(uint64_t entityID, glm::vec3* inTargetPosition, glm::vec3* inTargetRotation)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

			auto& component = entity.GetComponent<RigidBodyComponent>();
			if (!component.IsKinematic)
			{
				NR_CONSOLE_LOG_WARN("Cannot set kinematic target of a non-kinematic actor! EntityID({0})", entityID);
				return;
			}

			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			actor->SetKinematicTarget(*inTargetPosition, *inTargetRotation);
		}

		void NR_RigidBodyComponent_SetLockFlag(uint64_t entityID, ActorLockFlag flag, bool value)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

			auto& component = entity.GetComponent<RigidBodyComponent>();
			if (component.IsKinematic || component.BodyType == RigidBodyComponent::Type::Static)
			{
				NR_CONSOLE_LOG_WARN("Cannot set lock flags on a static or kinematic actor! EntityID({0})", entityID);
				return;
			}

			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			actor->SetLockFlag(flag, value);
		}

		bool NR_RigidBodyComponent_IsLockFlagSet(uint64_t entityID, ActorLockFlag flag)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

			auto& component = entity.GetComponent<RigidBodyComponent>();
			if (component.IsKinematic || component.BodyType == RigidBodyComponent::Type::Static)
				return false;

			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			return actor->IsLockFlagSet(flag);
		}

		uint32_t NR_RigidBodyComponent_GetLockFlags(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

			auto& component = entity.GetComponent<RigidBodyComponent>();
			if (component.IsKinematic || component.BodyType == RigidBodyComponent::Type::Static)
			{
				NR_CONSOLE_LOG_WARN("Cannot get lock flags from a static or kinematic actor! EntityID({0})", entityID);
				return 0;
			}

			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			NR_CORE_ASSERT(actor);
			return actor->GetLockFlags();
		}

		bool NR_RigidBodyComponent_IsKinematic(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

			auto& component = entity.GetComponent<RigidBodyComponent>();
			if (component.BodyType == RigidBodyComponent::Type::Static)
			{
				NR_CONSOLE_LOG_WARN("Static actors can't be kinematic! EntityID({0})", entityID);
				return false;
			}

			return ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity)->IsKinematic();
		}

		void NR_RigidBodyComponent_SetIsKinematic(uint64_t entityID, bool isKinematic)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

			auto& component = entity.GetComponent<RigidBodyComponent>();
			if (component.BodyType == RigidBodyComponent::Type::Static)
			{
				NR_CONSOLE_LOG_WARN("Cannot make a static actor kinematic! EntityID({0})", entityID);
				return;
			}

			ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity)->SetKinematic(isKinematic);
		}

		bool NR_RigidBodyComponent_IsSleeping(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

			auto& component = entity.GetComponent<RigidBodyComponent>();
			if (component.BodyType == RigidBodyComponent::Type::Static)
			{
				NR_CONSOLE_LOG_WARN("Shouldn't call RigidBodyComponent.IsSleeping on a static actor! EntityID({0})", entityID);
				return true;
			}

			return ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity)->IsSleeping();
		}

		void NR_RigidBodyComponent_SetIsSleeping(uint64_t entityID, bool isSleeping)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

			auto& component = entity.GetComponent<RigidBodyComponent>();
			if (component.BodyType == RigidBodyComponent::Type::Static)
			{
				NR_CONSOLE_LOG_WARN("Static actors can't be put to sleep! EntityID({0})", entityID);
				return;
			}

			auto& actor = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetActor(entity);
			if (isSleeping)
				actor->PutToSleep();
			else
				actor->WakeUp();
		}

		float NR_CharacterControllerComponent_GetSlopeLimit(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<CharacterControllerComponent>());
			auto& component = entity.GetComponent<CharacterControllerComponent>();
			return component.SlopeLimitDeg;
		}

		void NR_CharacterControllerComponent_SetSlopeLimit(uint64_t entityID, float slopeLimitDeg)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<CharacterControllerComponent>());

			auto& component = entity.GetComponent<CharacterControllerComponent>();

			auto& controller = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetController(entity);
			NR_CORE_ASSERT(controller);
			float slopeLimitDegClamped = std::clamp(slopeLimitDeg, 0.0f, 90.0f);
			controller->SetSlopeLimit(slopeLimitDegClamped);
			component.SlopeLimitDeg = slopeLimitDegClamped;
		}

		float NR_CharacterControllerComponent_GetStepOffset(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<CharacterControllerComponent>());
			auto& component = entity.GetComponent<CharacterControllerComponent>();
			return component.StepOffset;
		}

		void NR_CharacterControllerComponent_SetStepOffset(uint64_t entityID, float stepOffset)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<CharacterControllerComponent>());

			auto& component = entity.GetComponent<CharacterControllerComponent>();

			auto& controller = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetController(entity);
			NR_CORE_ASSERT(controller);
			controller->SetStepOffset(stepOffset);
			component.StepOffset = stepOffset;
		}

		void NR_CharacterControllerComponent_Move(uint64_t entityID, glm::vec3* displacement, float ts)
		{
			auto entity = GetEntity(entityID);
			auto& controller = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetController(entity);
			NR_CORE_ASSERT(controller);
			controller->Move(*displacement);
		}

		void NR_CharacterControllerComponent_GetVelocity(uint64_t entityID, glm::vec3* outResult)
		{
			auto entity = GetEntity(entityID);
			auto& controller = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetController(entity);
			NR_CORE_ASSERT(controller);
			*outResult = controller->GetVelocity();
		}

		bool NR_CharacterControllerComponent_IsGrounded(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			auto& controller = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetController(entity);
			NR_CORE_ASSERT(controller);
			return controller->IsGrounded();
		}

		CollisionFlags NR_CharacterControllerComponent_GetCollisionFlags(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			auto& controller = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetController(entity);
			NR_CORE_ASSERT(controller);
			return controller->GetCollisionFlags();
			NR_CORE_ASSERT(controller);
		}

		uint64_t NR_FixedJointComponent_GetConnectedEntity(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<FixedJointComponent>());

			auto& component = entity.GetComponent<FixedJointComponent>();
			return component.ConnectedEntity;
		}

		void NR_FixedJointComponent_SetConnectedEntity(uint64_t entityID, uint64_t connectedEntity)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<FixedJointComponent>());
			auto other = GetEntity(connectedEntity);

			auto& component = entity.GetComponent<FixedJointComponent>();
			component.ConnectedEntity = connectedEntity;

			ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetJoint(entity)->SetConnectedEntity(other);
		}

		bool NR_FixedJointComponent_IsBreakable(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<FixedJointComponent>());

			auto& component = entity.GetComponent<FixedJointComponent>();
			return component.IsBreakable;
		}

		void NR_FixedJointComponent_SetIsBreakable(uint64_t entityID, bool isBreakable)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<FixedJointComponent>());

			auto& component = entity.GetComponent<FixedJointComponent>();
			component.IsBreakable = isBreakable;

			auto& joint = ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetJoint(entity);
			if (isBreakable)
				joint->SetBreakForceAndTorque(component.BreakForce, component.BreakTorque);
			else
				joint->SetBreakForceAndTorque(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
		}

		bool NR_FixedJointComponent_IsBroken(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<FixedJointComponent>());

			return ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetJoint(entity)->IsBroken();
		}

		void NR_FixedJointComponent_Break(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<FixedJointComponent>());

			ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetJoint(entity)->Break();
		}

		float NR_FixedJointComponent_GetBreakForce(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<FixedJointComponent>());

			auto& component = entity.GetComponent<FixedJointComponent>();
			return component.BreakForce;
		}

		void NR_FixedJointComponent_SetBreakForce(uint64_t entityID, float breakForce)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<FixedJointComponent>());

			auto& component = entity.GetComponent<FixedJointComponent>();
			component.BreakForce = breakForce;
			ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetJoint(entity)->SetBreakForceAndTorque(component.BreakForce, component.BreakTorque);
		}

		float NR_FixedJointComponent_GetBreakTorque(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<FixedJointComponent>());

			auto& component = entity.GetComponent<FixedJointComponent>();
			return component.BreakTorque;
		}

		void NR_FixedJointComponent_SetBreakTorque(uint64_t entityID, float breakTorque)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<FixedJointComponent>());

			auto& component = entity.GetComponent<FixedJointComponent>();
			component.BreakTorque = breakTorque;
			ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetJoint(entity)->SetBreakForceAndTorque(component.BreakForce, component.BreakTorque);
		}

		bool NR_FixedJointComponent_IsCollisionEnabled(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<FixedJointComponent>());

			auto& component = entity.GetComponent<FixedJointComponent>();
			return component.EnableCollision;
		}

		void NR_FixedJointComponent_SetCollisionEnabled(uint64_t entityID, bool isCollisionEnabled)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<FixedJointComponent>());

			auto& component = entity.GetComponent<FixedJointComponent>();
			component.EnableCollision = isCollisionEnabled;
			ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetJoint(entity)->SetCollisionEnabled(isCollisionEnabled);
		}

		bool NR_FixedJointComponent_IsPreProcessingEnabled(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<FixedJointComponent>());

			auto& component = entity.GetComponent<FixedJointComponent>();
			return component.EnablePreProcessing;
		}

		void NR_FixedJointComponent_SetPreProcessingEnabled(uint64_t entityID, bool isPreProcessingEnabled)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<FixedJointComponent>());

			auto& component = entity.GetComponent<FixedJointComponent>();
			component.EnablePreProcessing = isPreProcessingEnabled;
			ScriptEngine::GetCurrentSceneContext()->GetPhysicsScene()->GetJoint(entity)->SetPreProcessingEnabled(isPreProcessingEnabled);
		}

		// TODO: Need better way of updating ColliderShape values!
		void NR_BoxColliderComponent_GetSize(uint64_t entityID, glm::vec3* outSize)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<BoxColliderComponent>());
			auto& component = entity.GetComponent<BoxColliderComponent>();
			*outSize = component.Size;
		}

		void NR_BoxColliderComponent_SetSize(uint64_t entityID, glm::vec3* inSize) {}

		void NR_BoxColliderComponent_GetOffset(uint64_t entityID, glm::vec3* outOffset)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<BoxColliderComponent>());
			auto& component = entity.GetComponent<BoxColliderComponent>();
			*outOffset = component.Offset;
		}

		void NR_BoxColliderComponent_SetOffset(uint64_t entityID, glm::vec3* inOffset) {}

		bool NR_BoxColliderComponent_IsTrigger(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<BoxColliderComponent>());
			auto& component = entity.GetComponent<BoxColliderComponent>();
			return component.IsTrigger;
		}

		void NR_BoxColliderComponent_SetTrigger(uint64_t entityID, bool isTrigger) {}

		float NR_SphereColliderComponent_GetRadius(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<SphereColliderComponent>());
			auto& component = entity.GetComponent<SphereColliderComponent>();
			return component.Radius;
		}

		void NR_SphereColliderComponent_SetRadius(uint64_t entityID, float inRadius) {}

		void NR_SphereColliderComponent_GetOffset(uint64_t entityID, glm::vec3* outOffset)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<SphereColliderComponent>());
			auto& component = entity.GetComponent<SphereColliderComponent>();
			*outOffset = component.Offset;
		}

		void NR_SphereColliderComponent_SetOffset(uint64_t entityID, glm::vec3* inOffset) {}

		bool NR_SphereColliderComponent_IsTrigger(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<SphereColliderComponent>());
			auto& component = entity.GetComponent<SphereColliderComponent>();
			return component.IsTrigger;
		}

		void NR_SphereColliderComponent_SetTrigger(uint64_t entityID, bool isTrigger) {}


		float NR_CapsuleColliderComponent_GetRadius(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<CapsuleColliderComponent>());
			auto& component = entity.GetComponent<CapsuleColliderComponent>();
			return component.Radius;
		}

		void NR_CapsuleColliderComponent_SetRadius(uint64_t entityID, float inRadius) {}

		float NR_CapsuleColliderComponent_GetHeight(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<CapsuleColliderComponent>());
			auto& component = entity.GetComponent<CapsuleColliderComponent>();
			return component.Height;
		}

		void NR_CapsuleColliderComponent_SetHeight(uint64_t entityID, float inHeight) {}

		void NR_CapsuleColliderComponent_GetOffset(uint64_t entityID, glm::vec3* outOffset)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<CapsuleColliderComponent>());
			auto& component = entity.GetComponent<CapsuleColliderComponent>();
			*outOffset = component.Offset;
		}

		void NR_CapsuleColliderComponent_SetOffset(uint64_t entityID, glm::vec3* inOffset) {}

		bool NR_CapsuleColliderComponent_IsTrigger(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<CapsuleColliderComponent>());
			auto& component = entity.GetComponent<CapsuleColliderComponent>();
			return component.IsTrigger;
		}

		void NR_CapsuleColliderComponent_SetTrigger(uint64_t entityID, bool isTrigger) {}

		Ref<Mesh>* NR_MeshColliderComponent_GetColliderMesh(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<MeshColliderComponent>());
			auto& component = entity.GetComponent<MeshColliderComponent>();
			auto mesh = AssetManager::GetAsset<Mesh>(component.CollisionMesh);
			return new Ref<Mesh>(new Mesh(mesh->GetMeshSource()));
		}

		void NR_MeshColliderComponent_SetColliderMesh(uint64_t entityID, Ref<Mesh>* inMesh) {}

		bool NR_MeshColliderComponent_IsConvex(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<MeshColliderComponent>());
			auto& component = entity.GetComponent<MeshColliderComponent>();
			return component.IsConvex;
		}

		void NR_MeshColliderComponent_SetConvex(uint64_t entityID, bool convex) {}

		bool NR_MeshColliderComponent_IsTrigger(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<MeshColliderComponent>());
			auto& component = entity.GetComponent<MeshColliderComponent>();
			return component.IsTrigger;
		}

		void NR_MeshColliderComponent_SetTrigger(uint64_t entityID, bool isTrigger) {}

		Ref<Mesh>* NR_Mesh_Constructor(MonoString* filepath)
		{
			AssetHandle handle = AssetManager::GetAssetHandleFromFilePath(mono_string_to_utf8(filepath));
			if (AssetManager::IsAssetHandleValid(handle))
				return new Ref<Mesh>(new Mesh(AssetManager::GetAsset<MeshSource>(handle)));

			return new Ref<Mesh>(new Mesh(Ref<MeshSource>::Create(mono_string_to_utf8(filepath))));
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
			if (!mesh->IsValid())
				return nullptr;

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
			// Convert RGBA32F color to RGBA8
			uint8_t* pixels = (uint8_t*)buffer.Data;
			uint32_t index = 0;
			for (uint32_t i = 0; i < instance->GetWidth() * instance->GetHeight(); i++)
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

		void NR_Material_GetEmission(Ref<MaterialAsset>* _this, float* outEmission)
		{
			Ref<MaterialAsset>& instance = *(Ref<MaterialAsset>*)_this;
			*outEmission = instance->GetEmission();
		}

		void NR_Material_SetEmission(Ref<MaterialAsset>* _this, float inEmission)
		{
			Ref<MaterialAsset>& instance = *(Ref<MaterialAsset>*)_this;
			instance->SetEmission(inEmission);
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
			// TODO: Implement properly with MeshFactory class!
			return new Ref<Mesh>(new Mesh(Ref<MeshSource>::Create("assets/models/Plane1m.obj")));
		}

		// TextComponent
		MonoString* NR_TextComponent_GetText(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<TextComponent>());
			auto& component = entity.GetComponent<TextComponent>();
			return mono_string_new(mono_domain_get(), component.TextString.c_str());
		}

		void NR_TextComponent_SetText(uint64_t entityID, MonoString* string)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<TextComponent>());
			auto& component = entity.GetComponent<TextComponent>();
			component.TextString = mono_string_to_utf8(string);
		}

		void NR_TextComponent_GetColor(uint64_t entityID, glm::vec4* outColor)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<TextComponent>());
			auto& component = entity.GetComponent<TextComponent>();
			*outColor = component.Color;
		}

		void NR_TextComponent_SetColor(uint64_t entityID, glm::vec4* inColor)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<TextComponent>());
			auto& component = entity.GetComponent<TextComponent>();
			component.Color = *inColor;
		}

		//====================================================================================
		//-------------------------------------- AUDIO ---------------------------------------
		//====================================================================================

		template<typename C >
		static inline C& GetEntityComponent(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<C>());
			return entity.GetComponent<C>();
		}

		//--- AudioComponent ---
		//======================

		bool NR_AudioComponent_IsPlaying(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<AudioComponent>());
			return AudioPlayback::IsPlaying(entityID);
		}

		bool NR_AudioComponent_Play(uint64_t entityID, float startTime)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<AudioComponent>());
			return AudioPlayback::Play(entityID, startTime);
		}

		bool NR_AudioComponent_Stop(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<AudioComponent>());
			return AudioPlayback::StopActiveSound(entityID);
		}

		bool NR_AudioComponent_Pause(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<AudioComponent>());
			return AudioPlayback::PauseActiveSound(entityID);
		}

		bool NR_AudioComponent_Resume(uint64_t entityID)
		{
			auto entity = GetEntity(entityID);
			NR_CORE_ASSERT(entity.HasComponent<AudioComponent>());
			return AudioPlayback::Resume(entityID);
		}

		float NR_AudioComponent_GetVolumeMult(uint64_t entityID)
		{
			return GetEntityComponent<AudioComponent>(entityID).VolumeMultiplier;
		}

		void NR_AudioComponent_SetVolumeMult(uint64_t entityID, float volumeMult)
		{
			GetEntityComponent<AudioComponent>(entityID).VolumeMultiplier = volumeMult;
		}

		float NR_AudioComponent_GetPitchMult(uint64_t entityID)
		{
			return GetEntityComponent<AudioComponent>(entityID).PitchMultiplier;
		}

		void NR_AudioComponent_SetPitchMult(uint64_t entityID, float pitchMult)
		{
			GetEntityComponent<AudioComponent>(entityID).PitchMultiplier = pitchMult;
		}

		void NR_AudioComponent_SetEvent(uint64_t entityID, Audio::CommandID eventID)
		{
			if (!AudioCommandRegistry::DoesCommandExist<Audio::TriggerCommand>(eventID))
			{
				NR_CORE_ERROR("Trigger Command with ID {0} does not exist!", eventID);
				return;
			}

			auto& audioComponent = GetEntityComponent<AudioComponent>(entityID);
			audioComponent.StartCommandID = eventID;
			audioComponent.StartEvent = AudioCommandRegistry::GetCommand<Audio::TriggerCommand>(eventID).DebugName;
		}

#if OLD_API
		float NR_AudioComponent_GetMasterReverbSend(uint64_t entityID)
		{
			return AudioPlayback::GetMasterReverbSend(entityID);
		}

		void NR_AudioComponent_SetMasterReverbSend(uint64_t entityID, float sendLevel)
		{
			AudioPlayback::SetMasterReverbSend(entityID, sendLevel);
		}
#endif

		//--- Create Sound ---
		//====================

		uint64_t NR_Audio_CreateSound(Audio::CommandID eventID, Audio::Transform* inSpawnLocation, float volume, float pitch)
		{
			auto scene = ScriptEngine::GetCurrentSceneContext();
			if (!AudioCommandRegistry::DoesCommandExist<Audio::TriggerCommand>(eventID))
			{
				NR_CORE_ERROR("Trigger Command with ID {0} does not exist!", eventID);
				return 0;
			}

			Entity entity = scene->CreateEntityWithID(NR::UUID(), "Sound3D");
			auto& audioComponent = entity.AddComponent<AudioComponent>();
			audioComponent.StartCommandID = eventID;
			audioComponent.StartEvent = AudioCommandRegistry::GetCommand<Audio::TriggerCommand>(eventID).DebugName;
			audioComponent.VolumeMultiplier = volume;
			audioComponent.PitchMultiplier = pitch;

			return entity.GetID();
		}

		uint32_t NR_Audio_CommandID_Constructor(MonoString* commandName)
		{
			return Audio::CommandID(mono_string_to_utf8(commandName));
		}

		uint32_t NR_Audio_PostEvent(Audio::CommandID eventID, uint64_t objectID)
		{
			return AudioPlayback::PostTrigger(eventID, objectID);
		}

		uint32_t NR_Audio_PostEventFromAC(Audio::CommandID eventID, uint64_t entityID)
		{
			GetEntityComponent<AudioComponent>(entityID).StartCommandID = eventID;

			return AudioPlayback::PostTriggerFromAC(eventID, entityID);
		}

		uint32_t NR_Audio_PostEventAtLocation(Audio::CommandID eventID, Audio::Transform* inSpawnLocation)
		{
			return AudioPlayback::PostTriggerAtLocation(eventID, *inSpawnLocation);
		}

		bool NR_Audio_StopEventID(uint32_t playingEvent)
		{
			return AudioPlayback::StopEventID(playingEvent);
		}

		bool NR_Audio_PauseEventID(uint32_t playingEvent)
		{
			return AudioPlayback::PauseEventID(playingEvent);
		}

		bool NR_Audio_ResumeEventID(uint32_t playingEvent)
		{
			return AudioPlayback::ResumeEventID(playingEvent);
		}

		//--- AudioObject ---
		//===================

		uint64_t NR_AudioObject_Constructor(MonoString* debugName, Audio::Transform* inObjectTransform)
		{
			return AudioPlayback::InitializeAudioObject(mono_string_to_utf8(debugName), *inObjectTransform);
		}

		void NR_ReleaseAudioObject(uint64_t objectID)
		{
			AudioPlayback::ReleaseAudioObject(objectID);
		}

		void NR_AudioObject_SetTransform(uint64_t objectID, Audio::Transform* inObjectTransform)
		{
			AudioEngine::Get().SetTransform(objectID, *inObjectTransform);
		}

		void NR_AudioObject_GetTransform(uint64_t objectID, Audio::Transform* outObjectTransform)
		{
			if (auto transform = AudioEngine::Get().GetTransform(objectID))
				*outObjectTransform = transform.value();
		}

		bool NR_Audio_GetObjectInfo(uint64_t objectID, MonoString* outDebugName)
		{
			auto objectInfo = AudioEngine::Get().GetAudioObjectInfo(objectID);
			if (!objectInfo.empty())
			{
				outDebugName = mono_string_new(mono_domain_get(), objectInfo.c_str());
				return true;
			}
			return false;
		}

		void NR_Log_LogMessage(LogLevel level, MonoString* message)
		{
			NR_PROFILE_FUNC();

			char* msg = mono_string_to_utf8(message);
			switch (level)
			{
			case LogLevel::Trace:
				NR_CONSOLE_LOG_TRACE(msg);
				break;
			case LogLevel::Debug:
				NR_CONSOLE_LOG_INFO(msg);
				break;
			case LogLevel::Info:
				NR_CONSOLE_LOG_INFO(msg);
				break;
			case LogLevel::Warn:
				NR_CONSOLE_LOG_WARN(msg);
				break;
			case LogLevel::Error:
				NR_CONSOLE_LOG_ERROR(msg);
				break;
			case LogLevel::Critical:
				NR_CONSOLE_LOG_FATAL(msg);
				break;
			}

		}
		////////////////////////////////////////////////////////////////
}