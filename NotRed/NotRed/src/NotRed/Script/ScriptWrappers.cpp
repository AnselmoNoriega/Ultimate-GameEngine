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
#include "NotRed/Scene/Prefab.h"

#include "NotRed/Audio/AudioComponent.h"
#include "NotRed/Audio/AudioPlayback.h"
#include "NotRed/Script/ScriptEngine.h"

#include "NotRed/Audio/AudioEngine.h"
#include "NotRed/Audio/AudioEvents/CommandID.h"
#include "NotRed/Audio/AudioEvents/AudioCommandRegistry.h"

#include "NotRed/Debug/Profiler.h"

#include <NotRed/Vendor/glm_vec3_json.hpp>

using json = nlohmann::json;
namespace nlohmann::json_abi_v3_11_3::detail
{
    void to_json(json& j, const NR::Vertex& vertex)
    {
        j = json{
            {"Position", vertex.Position},
            {"Normal", vertex.Normal},
            {"Tangent", vertex.Tangent},
            {"Binormal", vertex.Binormal},
            {"Texcoord", vertex.Texcoord}
        };
    }

    void from_json(const json& j, NR::Vertex& vertex)
    {
        j.at("Position").get_to(vertex.Position);
        j.at("Normal").get_to(vertex.Normal);
        j.at("Tangent").get_to(vertex.Tangent);
        j.at("Binormal").get_to(vertex.Binormal);
        j.at("Texcoord").get_to(vertex.Texcoord);
    }

    // Implement to_json and from_json for std::vector<Vertex>
    void to_json(json& j, const std::vector<NR::Vertex>& vertices)
    {
        j = json::array();
        for (const auto& vertex : vertices)
        {
            j.push_back(vertex);
        }
    }

    void from_json(const json& j, std::vector<NR::Vertex>& vertices)
    {
        vertices.clear();
        for (const auto& item : j)
        {
            vertices.push_back(item.get<NR::Vertex>());
        }
    }
}

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

    void NR_Entity_SetParent(uint64_t entityID, uint64_t parentID)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity parent = entityMap.at(parentID);
        Entity child = entityMap.at(entityID);
        child.SetParent(parent);
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

    uint64_t NR_Entity_Instantiate(uint64_t entityID, uint64_t prefabID)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        if (!AssetManager::IsAssetHandleValid(prefabID))
        {
            return 0;
        }

        Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(prefabID);
        Entity result = scene->Instantiate(prefab);
        return result.GetID();
    }

    uint64_t NR_Entity_InstantiateWithTranslation(uint64_t entityID, uint64_t prefabID, glm::vec3* inTranslation)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        if (!AssetManager::IsAssetHandleValid(prefabID))
        {
            return 0;
        }

        Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(prefabID);
        Entity result = scene->Instantiate(prefab, inTranslation);
        return result.GetID();
    }

    void NR_Entity_DestroyEntity(uint64_t entityID)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        scene->SubmitToDestroyEntity(entity);
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

        if (entity.HasComponent<RigidBodyComponent>())
        {
            auto actor = PhysicsManager::GetScene()->GetActor(entity);
            actor->SetPosition(*inTranslation);
        }
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

        if (entity.HasComponent<RigidBodyComponent>())
        {
            auto actor = PhysicsManager::GetScene()->GetActor(entity);
            actor->SetRotation(*inRotation);
        }
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
        NR_CORE_ASSERT(PhysicsManager::GetScene()->IsValid());

        RaycastHit temp;
        bool success = PhysicsManager::GetScene()->Raycast(inData->Origin, inData->Direction, inData->MaxDistance, &temp);
        if (success && inData->RequiredComponentTypes != nullptr)
        {
            Entity entity = scene->FindEntityByID(temp.HitEntity);
            size_t length = mono_array_length(inData->RequiredComponentTypes);
            for (size_t i = 0; i < length; ++i)
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

    void NR_Physics_GetGravity(glm::vec3* outGravity)
    {
        Ref<PhysicsScene> scene = PhysicsManager::GetScene();
        NR_CORE_ASSERT(scene && scene->IsValid(), "No active Phyiscs Scene!");
        *outGravity = scene->GetGravity();
    }

    void NR_Physics_SetGravity(glm::vec3* inGravity)
    {
        Ref<PhysicsScene> scene = PhysicsManager::GetScene();
        NR_CORE_ASSERT(scene && scene->IsValid(), "No active Phyiscs Scene!");
        scene->SetGravity(*inGravity);
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

    bool NR_MeshComponent_GetIsAnimated(uint64_t entityID)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        auto& meshComponent = entity.GetComponent<MeshComponent>();
        return meshComponent.MeshObj->IsAnimated();
    }

    bool NR_MeshComponent_GetIsAnimationPlaying(uint64_t entityID)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        auto& meshComponent = entity.GetComponent<MeshComponent>();
        return meshComponent.MeshObj->IsAnimationPlaying();
    }

    void NR_MeshComponent_SetIsAnimationPlaying(uint64_t entityID, bool value)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        auto& meshComponent = entity.GetComponent<MeshComponent>();
        meshComponent.MeshObj->SetAnimationPlaying(value);
    }

    bool NR_MeshComponent_HasMaterial(uint64_t entityID, int index)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        auto& meshComponent = entity.GetComponent<MeshComponent>();
        const auto& materialTable = meshComponent.Materials;

        return materialTable->HasMaterial(index);
    }

    Ref<MaterialAsset>* NR_MeshComponent_GetMaterial(uint64_t entityID, int index)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        auto& meshComponent = entity.GetComponent<MeshComponent>();
        const auto& materialTable = meshComponent.Materials;

        return new Ref<MaterialAsset>(materialTable->GetMaterial(index));
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

    void NR_RigidBodyComponent_GetPosition(uint64_t entityID, glm::vec3* outTranslation)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

        Ref<PhysicsActor> actor = PhysicsManager::GetScene()->GetActor(entity);
        *outTranslation = actor->GetPosition();
    }

    void NR_RigidBodyComponent_SetPosition(uint64_t entityID, glm::vec3* inTranslation)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());
        auto& component = entity.GetComponent<RigidBodyComponent>();

        Ref<PhysicsActor> actor = PhysicsManager::GetScene()->GetActor(entity);
        NR_CORE_ASSERT(actor);
        actor->SetPosition(*inTranslation);
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
        NR_CORE_ASSERT(actor);
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

    void NR_RigidBodyComponent_SetLockFlag(uint64_t entityID, ActorLockFlag flag, bool value)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

        auto& component = entity.GetComponent<RigidBodyComponent>();
        Ref<PhysicsActor> actor = PhysicsManager::GetScene()->GetActor(entity);
        actor->ModifyLockFlag(flag, value);
    }

    bool NR_RigidBodyComponent_IsLockFlagSet(uint64_t entityID, ActorLockFlag flag)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

        auto& component = entity.GetComponent<RigidBodyComponent>();
        Ref<PhysicsActor> actor = PhysicsManager::GetScene()->GetActor(entity);
        return actor->IsLockFlagSet(flag);
    }

    uint32_t NR_RigidBodyComponent_GetLockFlags(uint64_t entityID)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<RigidBodyComponent>());

        auto& component = entity.GetComponent<RigidBodyComponent>();
        Ref<PhysicsActor> actor = PhysicsManager::GetScene()->GetActor(entity);
        return actor->GetLockFlags();
    }

    void NR_BoxColliderComponent_GetSize(uint64_t entityID, glm::vec3* outSize)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<BoxColliderComponent>());

        auto& component = entity.GetComponent<BoxColliderComponent>();
        *outSize = component.Size;
    }

    void NR_BoxColliderComponent_SetSize(uint64_t entityID, glm::vec3* inSize) {}

    void NR_BoxColliderComponent_GetOffset(uint64_t entityID, glm::vec3* outOffset)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<BoxColliderComponent>());

        auto& component = entity.GetComponent<BoxColliderComponent>();
        *outOffset = component.Offset;
    }

    void NR_BoxColliderComponent_SetOffset(uint64_t entityID, glm::vec3* inOffset) {}

    bool NR_BoxColliderComponent_IsTrigger(uint64_t entityID)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<BoxColliderComponent>());

        auto& component = entity.GetComponent<BoxColliderComponent>();
        return component.IsTrigger;
    }
    void NR_BoxColliderComponent_SetTrigger(uint64_t entityID, bool isTrigger) {}

    float NR_SphereColliderComponent_GetRadius(uint64_t entityID)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<SphereColliderComponent>());

        auto& component = entity.GetComponent<SphereColliderComponent>();
        return component.Radius;
    }
    void NR_SphereColliderComponent_SetRadius(uint64_t entityID, float inRadius) {}

    void NR_SphereColliderComponent_GetOffset(uint64_t entityID, glm::vec3* outOffset)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<SphereColliderComponent>());

        auto& component = entity.GetComponent<SphereColliderComponent>();
        *outOffset = component.Offset;
    }
    void NR_SphereColliderComponent_SetOffset(uint64_t entityID, glm::vec3* inOffset) {}

    bool NR_SphereColliderComponent_IsTrigger(uint64_t entityID)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<SphereColliderComponent>());

        auto& component = entity.GetComponent<SphereColliderComponent>();
        return component.IsTrigger;
    }
    void NR_SphereColliderComponent_SetTrigger(uint64_t entityID, bool isTrigger) {}

    float NR_CapsuleColliderComponent_GetRadius(uint64_t entityID)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<CapsuleColliderComponent>());

        auto& component = entity.GetComponent<CapsuleColliderComponent>();
        return component.Radius;
    }
    void NR_CapsuleColliderComponent_SetRadius(uint64_t entityID, float inRadius) {}

    float NR_CapsuleColliderComponent_GetHeight(uint64_t entityID)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<CapsuleColliderComponent>());

        auto& component = entity.GetComponent<CapsuleColliderComponent>();
        return component.Height;
    }
    void NR_CapsuleColliderComponent_SetHeight(uint64_t entityID, float inHeight) {}

    void NR_CapsuleColliderComponent_GetOffset(uint64_t entityID, glm::vec3* outOffset)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<CapsuleColliderComponent>());

        auto& component = entity.GetComponent<CapsuleColliderComponent>();
        *outOffset = component.Offset;
    }

    void NR_CapsuleColliderComponent_SetOffset(uint64_t entityID, glm::vec3* inOffset) {}

    bool NR_CapsuleColliderComponent_IsTrigger(uint64_t entityID)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<CapsuleColliderComponent>());

        auto& component = entity.GetComponent<CapsuleColliderComponent>();
        return component.IsTrigger;
    }

    void NR_CapsuleColliderComponent_SetTrigger(uint64_t entityID, bool isTrigger) {}

    Ref<Mesh>* NR_MeshColliderComponent_GetColliderMesh(uint64_t entityID)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<MeshColliderComponent>());

        auto& component = entity.GetComponent<MeshColliderComponent>();
        return new Ref<Mesh>(new Mesh(component.CollisionMesh->GetMeshAsset()));
    }

    void NR_MeshColliderComponent_SetColliderMesh(uint64_t entityID, Ref<Mesh>* inMesh) {}

    bool NR_MeshColliderComponent_IsConvex(uint64_t entityID)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<MeshColliderComponent>());

        auto& component = entity.GetComponent<MeshColliderComponent>();
        return component.IsConvex;
    }

    void NR_MeshColliderComponent_SetConvex(uint64_t entityID, bool convex) {}

    bool NR_MeshColliderComponent_IsTrigger(uint64_t entityID)
    {
        Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
        NR_CORE_ASSERT(scene, "No active scene!");

        const auto& entityMap = scene->GetEntityMap();
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

        Entity entity = entityMap.at(entityID);
        NR_CORE_ASSERT(entity.HasComponent<MeshColliderComponent>());

        auto& component = entity.GetComponent<MeshColliderComponent>();
        return component.IsTrigger;
    }

    void NR_MeshColliderComponent_SetTrigger(uint64_t entityID, bool isTrigger) {}

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

    MonoArray* NR_Mesh_GetVertices(Ref<Mesh>* inMesh)
    {
        Ref<Mesh>& mesh = *(Ref<Mesh>*)inMesh;
        const auto& staticVertices = mesh->GetMeshAsset()->GetStaticVertices();

        json j = staticVertices;
        std::string serialized = j.dump();
        std::vector<byte> serializedData = std::vector<byte>(serialized.begin(), serialized.end());

        // Create a MonoArray to return to C#
        MonoClass* byteClass = mono_get_byte_class();
        MonoArray* result = mono_array_new(mono_domain_get(), byteClass, serializedData.size());
        for (size_t i = 0; i < serializedData.size(); ++i)
        {
            mono_array_set(result, uint8_t, i, serializedData[i]);
        }

        return result;
    }

    void NR_Mesh_SetVertices(Ref<Mesh>* inMesh, MonoArray* byteArray, int index)
    {
        int length = mono_array_length(byteArray);
        std::vector<byte> data(length);

        for (int i = 0; i < length; ++i)
        {
            data.push_back(mono_array_get(byteArray, byte, i));
        }

        std::string serialized(data.begin(), data.end());
        json j = json::parse(serialized);

        Ref<Mesh>& mesh = *(Ref<Mesh>*)inMesh;

        mesh->AddVertices(j.get<std::vector<Vertex>>(), index);
    }

    MonoArray* NR_Mesh_GetIndices(Ref<Mesh>* inMesh)
    {
        Ref<Mesh>& mesh = *(Ref<Mesh>*)inMesh;
        const auto& indices = mesh->GetMeshAsset()->GetIndices();
        MonoArray* array = mono_array_new(mono_domain_get(), mono_get_int32_class(), indices.size() * 3);

        size_t idx = 0;
        for (const auto& indexSet : indices)
        {
            mono_array_set(array, int32_t, idx++, indexSet.V1);
            mono_array_set(array, int32_t, idx++, indexSet.V2);
            mono_array_set(array, int32_t, idx++, indexSet.V3);
        }

        return array;
    }

    void NR_Mesh_SetIndices(Ref<Mesh>* inMesh, MonoArray* intArray, int index)
    {
        int length = mono_array_length(intArray);
        std::vector<int> data(length);

        for (int i = 0; i < length; ++i)
        {
            data.push_back(*(int*)mono_array_addr_with_size(intArray, sizeof(int), i));
        }

        Ref<Mesh>& mesh = *(Ref<Mesh>*)inMesh;
        mesh->AddIndices(data, index);
    }

    int NR_Mesh_GetSubMeshCount(Ref<Mesh>* inMesh)
    {
        Ref<Mesh>& mesh = *(Ref<Mesh>*)inMesh;
        return mesh->GetSubmeshes().size();
    }

    void NR_Mesh_SetSubMeshCount(Ref<Mesh>* inMesh, int count)
    {
        Ref<Mesh>& mesh = *(Ref<Mesh>*)inMesh;
        mesh->SetSubmeshesCount(count);
    }

    Ref<MaterialAsset>* NR_Mesh_GetMaterialByIndex(Ref<Mesh>* inMesh, int index)
    {
        Ref<Mesh>& mesh = *(Ref<Mesh>*)inMesh;
        if (!mesh->IsValid())
        {
            return nullptr;
        }
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
        return new Ref<Mesh>(new Mesh(Ref<MeshAsset>::Create("Assets/Models/Plane1m.obj")));
    }

    void* NR_MeshFactory_CreateCustomMesh(
        MonoArray* verticesArray,
        MonoArray* vertexOffsets,
        MonoArray* indicesArray,
        MonoArray* indexOffsets,
        MonoArray* matNames
    )
    {
        // Get the length of each MonoArray
        int vertexDataCount = mono_array_length(verticesArray);
        int vertexOffsetsCount = mono_array_length(vertexOffsets);
        int indicesCount = mono_array_length(indicesArray);
        int indexOffsetsCount = mono_array_length(indexOffsets);
        int matNamesCount = mono_array_length(matNames);

        // Get raw pointers from MonoArrays
        Vertex* verticesDataPtr = (Vertex*)(mono_array_addr_with_size(verticesArray, sizeof(Vertex), 0));
        int* vertexOffsetsPtr = (int*)mono_array_addr_with_size(vertexOffsets, sizeof(int), 0);
        int* indicesDataPtr = (int*)mono_array_addr_with_size(indicesArray, sizeof(int), 0);
        int* indexOffsetsPtr = (int*)mono_array_addr_with_size(indexOffsets, sizeof(int), 0);

        // Create vectors for offsets
        std::vector<int> vertexOffsetsVec(vertexOffsetsPtr, vertexOffsetsPtr + vertexOffsetsCount);
        std::vector<int> indexOffsetsVec(indexOffsetsPtr, indexOffsetsPtr + indexOffsetsCount);

        // Deserialize the vertex data and indices
        std::vector<std::vector<Vertex>> verticesForSubmeshes;
        verticesForSubmeshes.reserve(vertexOffsetsVec.size());
        {
            std::vector<Vertex> vertices(verticesDataPtr, verticesDataPtr + vertexDataCount);
            int endOffset = 0;
            for (int offset : vertexOffsetsVec)
            {
                verticesForSubmeshes.emplace_back(vertices.begin() + endOffset, vertices.begin() + offset);
                endOffset += offset;
            }
        }

        std::vector<std::vector<Index>> indicesForSubmeshes;
        indicesForSubmeshes.reserve(indexOffsetsVec.size());
        {
            std::vector<Index> indices;
            indices.resize(indicesCount / 3);
            std::memcpy(indices.data(), indicesDataPtr, indicesCount * sizeof(uint32_t));

            int endOffset = 0;
            for (int offset : indexOffsetsVec)
            {
                indicesForSubmeshes.emplace_back(indices.begin() + endOffset, indices.begin() + (offset / 3));
                endOffset += offset / 3;
            }
        }

        // Create vector for material names
        std::vector<std::string> materialNames;
        for (int i = 0; i < matNamesCount; ++i)
        {
            MonoString* monoStr = (MonoString*)mono_array_get(matNames, MonoString*, i);
            const char* cStr = mono_string_to_utf8(monoStr);

            materialNames.emplace_back(cStr);

            mono_free((void*)cStr);
        }

        return new Ref<Mesh>(new Mesh(Ref<MeshAsset>::Create(
            verticesForSubmeshes,
            indicesForSubmeshes,
            materialNames
        )));
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
        NR_CORE_ASSERT(entity.HasComponent<AudioComponent>());
        return AudioPlayback::IsPlaying(entityID);
    }

    bool NR_AudioComponent_Play(uint64_t entityID, float startTime)
    {
        Entity entity = GetEntityFromScene(entityID);
        NR_CORE_ASSERT(entity.HasComponent<AudioComponent>());
        return AudioPlayback::Play(entityID, startTime);
    }

    bool NR_AudioComponent_Stop(uint64_t entityID)
    {
        Entity entity = GetEntityFromScene(entityID);
        NR_CORE_ASSERT(entity.HasComponent<AudioComponent>());
        return AudioPlayback::StopActiveSound(entityID);
    }

    bool NR_AudioComponent_Pause(uint64_t entityID)
    {
        Entity entity = GetEntityFromScene(entityID);
        NR_CORE_ASSERT(entity.HasComponent<AudioComponent>());
        return AudioPlayback::PauseActiveSound(entityID);
    }

    bool NR_AudioComponent_Resume(uint64_t entityID)
    {
        Entity entity = GetEntityFromScene(entityID);
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
            NR_CORE_ERROR("Trigger Command with ID {0} does not exist!", (uint32_t)eventID);
            return;
        }

        auto& audioComponent = GetEntityComponent<AudioComponent>(entityID);
        audioComponent.StartCommandID = eventID;
        audioComponent.StartEvent = AudioCommandRegistry::GetCommand<Audio::TriggerCommand>(eventID).DebugName;
    }

    uint64_t NR_Audio_CreateSound(Audio::CommandID eventID, Audio::Transform* inSpawnLocation, float volume, float pitch)
    {
        auto scene = CheckActiveScene();
        if (!AudioCommandRegistry::DoesCommandExist<Audio::TriggerCommand>(eventID))
        {
            NR_CORE_ERROR("Trigger Command with ID {0} does not exist!", (uint32_t)eventID);
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
        {
            *outObjectTransform = transform.value();
        }
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
        case LogLevel::Trace:         NR_CONSOLE_LOG_TRACE(msg); break;
        case LogLevel::Debug:         NR_CONSOLE_LOG_INFO(msg); break;
        case LogLevel::Info:          NR_CONSOLE_LOG_INFO(msg); break;
        case LogLevel::Warn:          NR_CONSOLE_LOG_WARN(msg); break;
        case LogLevel::Error:         NR_CONSOLE_LOG_ERROR(msg); break;
        case LogLevel::Critical:      NR_CONSOLE_LOG_FATAL(msg); break;
        }
    }
}