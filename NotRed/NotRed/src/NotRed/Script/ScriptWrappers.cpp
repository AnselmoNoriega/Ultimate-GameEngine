#include "nrpch.h"
#include "ScriptWrappers.h"

#include <glm/gtc/type_ptr.hpp>
#include <mono/jit/jit.h>

#include "NotRed/Core/Math/Noise.h"

#include "NotRed/Scene/Scene.h"
#include "NotRed/Scene/Entity.h"
#include "NotRed/Scene/Components.h"

#include "NotRed/Core/Input.h"

namespace NR
{
    extern std::unordered_map<uint32_t, Scene*> sActiveScenes;
    extern std::unordered_map<MonoType*, std::function<bool(Entity&)>> sHasComponentFuncs;
    extern std::unordered_map<MonoType*, std::function<void(Entity&)>> sCreateComponentFuncs;
}

namespace NR::Script
{
    enum class ComponentID
    {
        None,
        Transform,
        Mesh,
        Script,
        SpriteRenderer
    };



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

    ////////////////////////////////////////////////////////////////
    // Entity //////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////

    void NR_Entity_GetTransform(uint32_t sceneID, uint32_t entityID, glm::mat4* outTransform)
    {
        NR_CORE_ASSERT(sActiveScenes.find(sceneID) != sActiveScenes.end(), "Invalid Scene ID!");

        Scene* scene = sActiveScenes[sceneID];
        Entity entity((entt::entity)entityID, scene);
        auto& transformComponent = entity.GetComponent<TransformComponent>();
        memcpy(outTransform, glm::value_ptr(transformComponent.Transform), sizeof(glm::mat4));
    }

    void NR_Entity_SetTransform(uint32_t sceneID, uint32_t entityID, glm::mat4* inTransform)
    {
        NR_CORE_ASSERT(sActiveScenes.find(sceneID) != sActiveScenes.end(), "Invalid Scene ID!");

        Scene* scene = sActiveScenes[sceneID];
        Entity entity((entt::entity)entityID, scene);
        auto& transformComponent = entity.GetComponent<TransformComponent>();
        memcpy(glm::value_ptr(transformComponent.Transform), inTransform, sizeof(glm::mat4));
    }

    void NR_Entity_CreateComponent(uint32_t sceneID, uint32_t entityID, void* type)
    {
        NR_CORE_ASSERT(sActiveScenes.find(sceneID) != sActiveScenes.end(), "Invalid Scene ID!");
        MonoType* monoType = mono_reflection_type_get_type((MonoReflectionType*)type);

        Scene* scene = sActiveScenes[sceneID];
        Entity entity((entt::entity)entityID, scene);
        sCreateComponentFuncs[monoType](entity);
    }

    bool NR_Entity_HasComponent(uint32_t sceneID, uint32_t entityID, void* type)
    {
        NR_CORE_ASSERT(sActiveScenes.find(sceneID) != sActiveScenes.end(), "Invalid Scene ID!");
        MonoType* monoType = mono_reflection_type_get_type((MonoReflectionType*)type);

        Scene* scene = sActiveScenes[sceneID];
        Entity entity((entt::entity)entityID, scene);
        bool result = sHasComponentFuncs[monoType](entity);
        return result;
    }

    void* NR_MeshComponent_GetMesh(uint32_t sceneID, uint32_t entityID)
    {
        NR_CORE_ASSERT(sActiveScenes.find(sceneID) != sActiveScenes.end(), "Invalid Scene ID!");

        Scene* scene = sActiveScenes[sceneID];
        Entity entity((entt::entity)entityID, scene);
        auto& meshComponent = entity.GetComponent<MeshComponent>();
        return new Ref<Mesh>(meshComponent.MeshObj);
    }

    void NR_MeshComponent_SetMesh(uint32_t sceneID, uint32_t entityID, Ref<Mesh>* inMesh)
    {
        NR_CORE_ASSERT(sActiveScenes.find(sceneID) != sActiveScenes.end(), "Invalid Scene ID!");

        Scene* scene = sActiveScenes[sceneID];
        Entity entity((entt::entity)entityID, scene);
        auto& meshComponent = entity.GetComponent<MeshComponent>();
        meshComponent.MeshObj = inMesh ? *inMesh : nullptr;
    }

    Ref<Mesh>* NR_Mesh_Constructor(MonoString* filepath)
    {
        return new Ref<Mesh>(new Mesh(mono_string_to_utf8(filepath)));
    }

    void NR_Mesh_Destructor(Ref<Mesh>* _this)
    {
        Ref<Mesh>* instance = (Ref<Mesh>*)_this;
        delete _this;
    }

    Ref<Material>* NR_Mesh_GetMaterial(Ref<Mesh>* inMesh)
    {
        Ref<Mesh>& mesh = *(Ref<Mesh>*)inMesh;
        return new Ref<Material>(mesh->GetMaterial());
    }

    Ref<MaterialInstance>* NR_Mesh_GetMaterialByIndex(Ref<Mesh>* inMesh, int index)
    {
        Ref<Mesh>& mesh = *(Ref<Mesh>*)inMesh;
        const auto& materials = mesh->GetMaterials();

        NR_CORE_ASSERT(index < materials.size());
        return new Ref<MaterialInstance>(materials[index]);
    }

    int NR_Mesh_GetMaterialCount(Ref<Mesh>* inMesh)
    {
        Ref<Mesh>& mesh = *(Ref<Mesh>*)inMesh;
        const auto& materials = mesh->GetMaterials();
        return materials.size();
    }

    void* NR_Texture2D_Constructor(uint32_t width, uint32_t height)
    {
        auto result = Texture2D::Create(TextureFormat::RGBA, width, height);
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
        for (int i = 0; i < instance->GetWidth() * instance->GetHeight(); i++)
        {
            glm::vec4& value = mono_array_get(inData, glm::vec4, i);
            *pixels++ = (uint32_t)(value.x * 255.0f);
            *pixels++ = (uint32_t)(value.y * 255.0f);
            *pixels++ = (uint32_t)(value.z * 255.0f);
            *pixels++ = (uint32_t)(value.w * 255.0f);
        }

        instance->Unlock();
    }

    void NR_Material_Destructor(Ref<Material>* _this)
    {
        delete _this;
    }

    void NR_Material_SetFloat(Ref<Material>* _this, MonoString* uniform, float value)
    {
        Ref<Material>& instance = *(Ref<Material>*)_this;
        instance->Set(mono_string_to_utf8(uniform), value);
    }

    void NR_Material_SetTexture(Ref<Material>* _this, MonoString* uniform, Ref<Texture2D>* texture)
    {
        Ref<Material>& instance = *(Ref<Material>*)_this;
        instance->Set(mono_string_to_utf8(uniform), *texture);
    }

    void NR_MaterialInstance_Destructor(Ref<MaterialInstance>* _this)
    {
        delete _this;
    }

    void NR_MaterialInstance_SetFloat(Ref<MaterialInstance>* _this, MonoString* uniform, float value)
    {
        Ref<MaterialInstance>& instance = *(Ref<MaterialInstance>*)_this;
        instance->Set(mono_string_to_utf8(uniform), value);
    }

    void NR_MaterialInstance_SetVector3(Ref<MaterialInstance>* _this, MonoString* uniform, glm::vec3* value)
    {
        Ref<MaterialInstance>& instance = *(Ref<MaterialInstance>*)_this;
        instance->Set(mono_string_to_utf8(uniform), *value);
    }

    void NR_MaterialInstance_SetTexture(Ref<MaterialInstance>* _this, MonoString* uniform, Ref<Texture2D>* texture)
    {
        Ref<MaterialInstance>& instance = *(Ref<MaterialInstance>*)_this;
        instance->Set(mono_string_to_utf8(uniform), *texture);
    }

    void* NR_MeshFactory_CreatePlane(float width, float height)
    {
        return new Ref<Mesh>(new Mesh("Assets/Models/Plane1m.obj"));
    }
}