#include "nrpch.h"
#include "ScriptEngine.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/attrdefs.h>

#include <iostream>
#include <chrono>
#include <thread>

#include "imgui.h"

#include "ScriptEngineRegistry.h"

#include "NotRed/Scene/Scene.h"

#include "NotRed/Debug/Profiler.h"

namespace NR
{
    static MonoDomain* sMonoDomain = nullptr;
    static std::string sAssemblyPath;
    static Ref<Scene> sSceneContext;

    MonoImage* sAppAssemblyImage = nullptr;
    MonoImage* sCoreAssemblyImage = nullptr;

    static EntityInstanceMap sEntityInstanceMap;

    static MonoMethod* GetMethod(MonoImage* image, const std::string& methodDesc);

    struct EntityScriptClass
    {
        std::string FullName;
        std::string ClassName;
        std::string NamespaceName;

        MonoClass* Class = nullptr;
        MonoMethod* Constructor = nullptr;
        MonoMethod* InitMethod = nullptr;
        MonoMethod* DestroyMethod = nullptr;
        MonoMethod* UpdateMethod = nullptr;
        MonoMethod* UpdatePhysics = nullptr;

        MonoMethod* Collision2DBeginMethod = nullptr;
        MonoMethod* Collision2DEndMethod = nullptr;
        MonoMethod* CollisionBeginMethod = nullptr;
        MonoMethod* CollisionEndMethod = nullptr;
		MonoMethod* TriggerBeginMethod = nullptr;
		MonoMethod* TriggerEndMethod = nullptr;

        void InitClassMethods(MonoImage* image)
        {
            Constructor = GetMethod(sCoreAssemblyImage, "NR.Entity:.ctor(ulong)");
            InitMethod = GetMethod(image, ClassName + ":Init()");
            UpdateMethod = GetMethod(image, ClassName + ":Update(single)");
            UpdatePhysics = GetMethod(image, FullName + ":UpdatePhysics(single)");

            Collision2DBeginMethod = GetMethod(sCoreAssemblyImage, "NR.Entity:Collision2DBegin(single)");
            Collision2DEndMethod = GetMethod(sCoreAssemblyImage, "NR.Entity:Collision2DEnd(single)");
            CollisionBeginMethod = GetMethod(sCoreAssemblyImage, "NR.Entity:CollisionBegin(single)");
            CollisionEndMethod = GetMethod(sCoreAssemblyImage, "NR.Entity:CollisionEnd(single)");
            TriggerBeginMethod = GetMethod(sCoreAssemblyImage, "NR.Entity:OnTriggerBegin(single)");
            TriggerEndMethod = GetMethod(sCoreAssemblyImage, "NR.Entity:OnTriggerEnd(single)");
        }
    };

    MonoObject* EntityInstance::GetInstance()
    {
        NR_CORE_ASSERT(Handle, "Entity has not been initiated!")
            return mono_gchandle_get_target(Handle);
    };

    static std::unordered_map<std::string, EntityScriptClass> sEntityClassMap;

    MonoAssembly* LoadAssemblyFromFile(const char* filepath)
    {
        if (filepath == NULL)
        {
            return NULL;
        }

        HANDLE file = CreateFileA(filepath, FILE_READ_ACCESS, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (file == INVALID_HANDLE_VALUE)
        {
            return NULL;
        }

        DWORD file_size = GetFileSize(file, NULL);
        if (file_size == INVALID_FILE_SIZE)
        {
            CloseHandle(file);
            return NULL;
        }

        void* file_data = malloc(file_size);
        if (file_data == NULL)
        {
            CloseHandle(file);
            return NULL;
        }

        DWORD read = 0;
        ReadFile(file, file_data, file_size, &read, NULL);
        if (file_size != read)
        {
            free(file_data);
            CloseHandle(file);
            return NULL;
        }

        MonoImageOpenStatus status;
        MonoImage* image = mono_image_open_from_data_full(reinterpret_cast<char*>(file_data), file_size, 1, &status, 0);
        if (status != MONO_IMAGE_OK)
        {
            return NULL;
        }
        auto assemb = mono_assembly_load_from_full(image, filepath, &status, 0);
        free(file_data);
        CloseHandle(file);
        mono_image_close(image);
        return assemb;
    }

    static void ShutdownMono()
    {
    }

    static void InitMono()
    {
        if (!sMonoDomain)
        {
            mono_set_assemblies_path("mono/lib");
            auto domain = mono_jit_init("NotRed");

            char* name = (char*)"NotRedRuntime";
            sMonoDomain = mono_domain_create_appdomain(name, nullptr);
        }
    }

    static MonoAssembly* LoadAssembly(const std::string& path)
    {
        MonoAssembly* assembly = LoadAssemblyFromFile(path.c_str());

        if (!assembly)
        {
            std::cout << "Could not load assembly: " << path << std::endl;
        }
        else
        {
            std::cout << "Successfully loaded assembly: " << path << std::endl;
        }

        return assembly;
    }

    static MonoImage* GetAssemblyImage(MonoAssembly* assembly)
    {
        MonoImage* image = mono_assembly_get_image(assembly);
        if (!image)
        {
            std::cout << "mono_assembly_get_image failed" << std::endl;
        }

        return image;
    }

    static MonoClass* GetClass(MonoImage* image, const EntityScriptClass& scriptClass)
    {
        MonoClass* monoClass = mono_class_from_name(image, scriptClass.NamespaceName.c_str(), scriptClass.ClassName.c_str());
        if (!monoClass)
        {
            std::cout << "mono_classfromname failed" << std::endl;
        }

        return monoClass;
    }

    static uint32_t Instantiate(EntityScriptClass& scriptClass)
    {
        MonoObject* instance = mono_object_new(sMonoDomain, scriptClass.Class);
        if (!instance)
        {
            std::cout << "mono_object_new failed" << std::endl;
        }

        mono_runtime_object_init(instance);
        uint32_t handle = mono_gchandle_new(instance, false);
        return handle;
    }

    static MonoMethod* GetMethod(MonoImage* image, const std::string& methodDesc)
    {
        MonoMethodDesc* desc = mono_method_desc_new(methodDesc.c_str(), NULL);
        if (!desc)
        {
            NR_CORE_ERROR("[ScriptEngine] mono_method_desc_new failed ({0})", methodDesc);
        }

        MonoMethod* method = mono_method_desc_search_in_image(desc, image);
        if (!method)
        {
            NR_CORE_ERROR("[ScriptEngine] mono_method_desc_search_in_image failed ({0})", methodDesc);
        }

        return method;
    }

    static MonoObject* CallMethod(MonoObject* object, MonoMethod* method, void** params = nullptr)
    {
        MonoObject* pException = NULL;
        MonoObject* result = mono_runtime_invoke(method, object, params, &pException);
        return result;
    }

    static void PrintClassMethods(MonoClass* monoClass)
    {
        MonoMethod* iter;
        void* ptr = 0;
        while ((iter = mono_class_get_methods(monoClass, &ptr)) != NULL)
        {
            printf("--------------------------------\n");
            const char* name = mono_method_get_name(iter);
            MonoMethodDesc* methodDesc = mono_method_desc_from_method(iter);

            const char* paramNames = "";
            mono_method_get_param_names(iter, &paramNames);

            printf("Name: %s\n", name);
            printf("Full name: %s\n", mono_method_full_name(iter, true));
        }
    }

    static void PrintClassProperties(MonoClass* monoClass)
    {
        MonoProperty* iter;
        void* ptr = 0;
        while ((iter = mono_class_get_properties(monoClass, &ptr)) != NULL)
        {
            printf("--------------------------------\n");
            const char* name = mono_property_get_name(iter);

            printf("Name: %s\n", name);
        }
    }

    static MonoAssembly* sAppAssembly = nullptr;
    static MonoAssembly* sCoreAssembly = nullptr;

    static MonoString* GetName()
    {
        return mono_string_new(sMonoDomain, "Hello!");
    }

    void ScriptEngine::LoadRuntimeAssembly(const std::string& path)
    {
        MonoDomain* domain = nullptr;
        bool cleanup = false;
        if (sMonoDomain)
        {
            domain = mono_domain_create_appdomain((char*)"NotRed Runtime", nullptr);
            mono_domain_set(domain, false);

            cleanup = true;
        }

        sCoreAssembly = LoadAssembly("Assets/Scripts/NotRed-ScriptCore.dll");
        sCoreAssemblyImage = GetAssemblyImage(sCoreAssembly);

        auto appAssembly = LoadAssembly(path);
        auto appAssemblyImage = GetAssemblyImage(appAssembly);
        ScriptEngineRegistry::RegisterAll();

        if (cleanup)
        {
            mono_domain_unload(sMonoDomain);
            sMonoDomain = domain;
        }

        sAppAssembly = appAssembly;
        sAppAssemblyImage = appAssemblyImage;
    }

    void ScriptEngine::ReloadAssembly(const std::string& path)
    {
        LoadRuntimeAssembly(path);
        if (sEntityInstanceMap.size())
        {
            Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
            NR_CORE_ASSERT(scene, "No active scene!");
            if (sEntityInstanceMap.find(scene->GetID()) != sEntityInstanceMap.end())
            {
                auto& entityMap = sEntityInstanceMap.at(scene->GetID());
                for (auto& [entityID, entityInstanceData] : entityMap)
                {
                    const auto& entityMap = scene->GetEntityMap();
                    NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");
                    InitScriptEntity(entityMap.at(entityID));
                }
            }
        }
    }

    void ScriptEngine::Init(const std::string& assemblyPath)
    {
        sAssemblyPath = assemblyPath;

        InitMono();

        LoadRuntimeAssembly(sAssemblyPath);
    }

    void ScriptEngine::Shutdown()
    {
        ShutdownMono();
        sSceneContext = nullptr;
        sEntityInstanceMap.clear();
    }

    void ScriptEngine::SceneDestruct(UUID sceneID)
    {
        if (sEntityInstanceMap.find(sceneID) != sEntityInstanceMap.end())
        {
            sEntityInstanceMap.at(sceneID).clear();
            sEntityInstanceMap.erase(sceneID);
        }
    }

    void ScriptEngine::SetSceneContext(const Ref<Scene>& scene)
    {
        sSceneContext = scene;
    }

    const Ref<Scene>& ScriptEngine::GetCurrentSceneContext()
    {
        return sSceneContext;
    }

    void ScriptEngine::CopyEntityScriptData(UUID dst, UUID src)
    {
        NR_CORE_ASSERT(sEntityInstanceMap.find(dst) != sEntityInstanceMap.end());
        NR_CORE_ASSERT(sEntityInstanceMap.find(src) != sEntityInstanceMap.end());

        auto& dstEntityMap = sEntityInstanceMap.at(dst);
        auto& srcEntityMap = sEntityInstanceMap.at(src);

        for (auto& [entityID, entityInstanceData] : srcEntityMap)
        {
            for (auto& [moduleName, srcFieldMap] : srcEntityMap[entityID].ModuleFieldMap)
            {
                auto& dstModuleFieldMap = dstEntityMap[entityID].ModuleFieldMap;
                for (auto& [fieldName, field] : srcFieldMap)
                {
                    NR_CORE_ASSERT(dstModuleFieldMap.find(moduleName) != dstModuleFieldMap.end());
                    auto& fieldMap = dstModuleFieldMap.at(moduleName);
                    NR_CORE_ASSERT(fieldMap.find(fieldName) != fieldMap.end());
                    fieldMap.at(fieldName).SetStoredValueRaw(field.mStoredValueBuffer);
                }
            }
        }
    }

    void ScriptEngine::CreateEntity(Entity entity)
    {
        EntityInstance& entityInstance = GetEntityInstanceData(entity.GetSceneID(), entity.GetID()).Instance;
        if (entityInstance.ScriptClass->InitMethod)
        {
            CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->InitMethod);
        }
    }

    void ScriptEngine::UpdateEntity(Entity entity, float dt)
    {
        NR_PROFILE_FUNC();

        EntityInstance& entityInstance = GetEntityInstanceData(entity.GetSceneID(), entity.GetID()).Instance;
        NR_PROFILE_SCOPE_DYNAMIC(entityInstance.ScriptClass->FullName.c_str());
        if (entityInstance.ScriptClass->UpdateMethod)
        {
            void* args[] = { &dt };
            CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->UpdateMethod, args);
        }
    }

    void ScriptEngine::UpdatePhysicsEntity(Entity entity, float fixedDeltaTime)
    {
        NR_PROFILE_FUNC();

        EntityInstance& entityInstance = GetEntityInstanceData(entity.GetSceneID(), entity.GetID()).Instance;
        if (entityInstance.ScriptClass->UpdatePhysics)
        {
            void* args[] = { &fixedDeltaTime };
            CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->UpdatePhysics, args);
        }
    }

    void ScriptEngine::Collision2DBegin(Entity entity)
    {
        NR_PROFILE_FUNC();

        EntityInstance& entityInstance = GetEntityInstanceData(entity.GetSceneID(), entity.GetID()).Instance;
        if (entityInstance.ScriptClass->Collision2DBeginMethod)
        {
            float value = 5.0f;
            void* args[] = { &value };
            CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->Collision2DBeginMethod, args);
        }
    }

    void ScriptEngine::Collision2DEnd(Entity entity)
    {
        NR_PROFILE_FUNC();

        EntityInstance& entityInstance = GetEntityInstanceData(entity.GetSceneID(), entity.GetID()).Instance;
        if (entityInstance.ScriptClass->Collision2DEndMethod)
        {
            float value = 5.0f;
            void* args[] = { &value };
            CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->Collision2DEndMethod, args);
        }
    }

    void ScriptEngine::CollisionBegin(Entity entity)
    {
        NR_PROFILE_FUNC();

        EntityInstance& entityInstance = GetEntityInstanceData(entity.GetSceneID(), entity.GetID()).Instance;
        if (entityInstance.ScriptClass->CollisionBeginMethod)
        {
            float value = 5.0f;
            void* args[] = { &value };
            CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->CollisionBeginMethod, args);
        }
    }

    void ScriptEngine::CollisionEnd(Entity entity)
    {
        NR_PROFILE_FUNC();

        EntityInstance& entityInstance = GetEntityInstanceData(entity.GetSceneID(), entity.GetID()).Instance;
        if (entityInstance.ScriptClass->CollisionEndMethod)
        {
            float value = 5.0f;
            void* args[] = { &value };
            CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->CollisionEndMethod, args);
        }
    }

    void ScriptEngine::TriggerBegin(Entity entity)
    {
        NR_PROFILE_FUNC();

        EntityInstance& entityInstance = GetEntityInstanceData(entity.GetSceneID(), entity.GetID()).Instance;
        if (entityInstance.ScriptClass->TriggerBeginMethod)
        {
            float value = 5.0f;
            void* args[] = { &value };
            CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->TriggerBeginMethod, args);
        }
    }

    void ScriptEngine::TriggerEnd(Entity entity)
    {
        NR_PROFILE_FUNC();

        EntityInstance& entityInstance = GetEntityInstanceData(entity.GetSceneID(), entity.GetID()).Instance;
        if (entityInstance.ScriptClass->TriggerEndMethod)
        {
            float value = 5.0f;
            void* args[] = { &value };
            CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->TriggerEndMethod, args);
        }
    }

    MonoObject* ScriptEngine::Construct(const std::string& fullName, bool callConstructor, void** parameters)
    {
        std::string namespaceName;
        std::string className;
        std::string parameterList;

        if (fullName.find(".") != std::string::npos)
        {
            namespaceName = fullName.substr(0, fullName.find_first_of('.'));
            className = fullName.substr(fullName.find_first_of('.') + 1, (fullName.find_first_of(':') - fullName.find_first_of('.')) - 1);
        }

        if (fullName.find(":") != std::string::npos)
        {
            parameterList = fullName.substr(fullName.find_first_of(':'));
        }

        MonoClass* monoClass = mono_class_from_name(sCoreAssemblyImage, namespaceName.c_str(), className.c_str());
        MonoObject* obj = mono_object_new(mono_domain_get(), monoClass);

        if (callConstructor)
        {
            MonoMethodDesc* desc = mono_method_desc_new(parameterList.c_str(), NULL);
            MonoMethod* constructor = mono_method_desc_search_in_class(desc, monoClass);

            MonoObject* exception = nullptr;
            mono_runtime_invoke(constructor, obj, parameters, &exception);
        }

        return obj;
    }

    static std::unordered_map<std::string, MonoClass*> sClasses;

    MonoClass* ScriptEngine::GetCoreClass(const std::string& fullName)
    {
        if (sClasses.find(fullName) != sClasses.end())
        {
            return sClasses[fullName];
        }

        std::string namespaceName = "";
        std::string className;

        if (fullName.find('.') != std::string::npos)
        {
            namespaceName = fullName.substr(0, fullName.find_last_of('.'));
            className = fullName.substr(fullName.find_last_of('.') + 1);
        }
        else
        {
            className = fullName;
        }

        MonoClass* monoClass = mono_class_from_name(sCoreAssemblyImage, namespaceName.c_str(), className.c_str());
        if (!monoClass)
        {
            std::cout << "mono_class_from_name failed" << std::endl;
        }

        sClasses[fullName] = monoClass;

        return monoClass;
    }

    bool ScriptEngine::IsEntityModuleValid(Entity entity)
    {
        return entity.HasComponent<ScriptComponent>() && ModuleExists(entity.GetComponent<ScriptComponent>().ModuleName);
    }

    void ScriptEngine::ScriptComponentDestroyed(UUID sceneID, UUID entityID)
    {
        NR_CORE_ASSERT(sEntityInstanceMap.find(sceneID) != sEntityInstanceMap.end());
        auto& entityMap = sEntityInstanceMap.at(sceneID);
        NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end());
        entityMap.erase(entityID);
    }

    bool ScriptEngine::ModuleExists(const std::string& moduleName)
    {
        NR_PROFILE_FUNC();

        std::string NamespaceName, ClassName;
        if (moduleName.find('.') != std::string::npos)
        {
            NamespaceName = moduleName.substr(0, moduleName.find_last_of('.'));
            ClassName = moduleName.substr(moduleName.find_last_of('.') + 1);
        }
        else
        {
            ClassName = moduleName;
        }

        MonoClass* monoClass = mono_class_from_name(sAppAssemblyImage, NamespaceName.c_str(), ClassName.c_str());
        return monoClass != nullptr;
    }

    static FieldType GetFieldType(MonoType* monoType)
    {
        int type = mono_type_get_type(monoType);
        switch (type)
        {
        case MONO_TYPE_I4:			return FieldType::Int;
        case MONO_TYPE_R4:			return FieldType::Float;
        case MONO_TYPE_U4:			return FieldType::UnsignedInt;
        case MONO_TYPE_STRING:		return FieldType::String;
        case MONO_TYPE_CLASS:       return FieldType::ClassReference;
        case MONO_TYPE_VALUETYPE:
        {
            char* name = mono_type_get_name(monoType);
            if (strcmp(name, "NR.Vector2") == 0)	 return FieldType::Vec2;
            if (strcmp(name, "NR.Vector3") == 0)	 return FieldType::Vec3;
            if (strcmp(name, "NR.Vector4") == 0)	 return FieldType::Vec4;
        }
        }
        return FieldType::None;
    }

    const char* FieldTypeToString(FieldType type)
    {
        switch (type)
        {
        case FieldType::Float:       return "Float";
        case FieldType::Int:         return "Int";
        case FieldType::UnsignedInt: return "UnsignedInt";
        case FieldType::String:      return "String";
        case FieldType::Vec2:        return "Vec2";
        case FieldType::Vec3:        return "Vec3";
        case FieldType::Vec4:        return "Vec4";
        }
        return "Unknown";
    }

    void ScriptEngine::InitScriptEntity(Entity entity)
    {
        Scene* scene = entity.mScene;
        UUID id = entity.GetComponent<IDComponent>().ID;
        auto& moduleName = entity.GetComponent<ScriptComponent>().ModuleName;
        if (moduleName.empty())
        {
            return;
        }

        if (!ModuleExists(moduleName))
        {
            NR_CORE_ERROR("Entity references non-existent script module '{0}'", moduleName);
            return;
        }

        EntityScriptClass& scriptClass = sEntityClassMap[moduleName];
        scriptClass.FullName = moduleName;
        if (moduleName.find('.') != std::string::npos)
        {
            scriptClass.NamespaceName = moduleName.substr(0, moduleName.find_last_of('.'));
            scriptClass.ClassName = moduleName.substr(moduleName.find_last_of('.') + 1);
        }
        else
        {
            scriptClass.ClassName = moduleName;
        }

        scriptClass.Class = GetClass(sAppAssemblyImage, scriptClass);
        scriptClass.InitClassMethods(sAppAssemblyImage);

        EntityInstanceData& entityInstanceData = sEntityInstanceMap[scene->GetID()][id];
        EntityInstance& entityInstance = entityInstanceData.Instance;
        entityInstance.ScriptClass = &scriptClass;
        ScriptModuleFieldMap& moduleFieldMap = entityInstanceData.ModuleFieldMap;
        auto& fieldMap = moduleFieldMap[moduleName];

        std::unordered_map<std::string, PublicField> oldFields;
        oldFields.reserve(fieldMap.size());
        for (auto& [fieldName, field] : fieldMap)
        {
            oldFields.emplace(fieldName, std::move(field));
        }
        fieldMap.clear();

        {
            MonoClassField* iter;
            void* ptr = 0;
            while ((iter = mono_class_get_fields(scriptClass.Class, &ptr)) != NULL)
            {
                const char* name = mono_field_get_name(iter);
                uint32_t flags = mono_field_get_flags(iter);
                if ((flags & MONO_FIELD_ATTR_PUBLIC) == 0)
                {
                    continue;
                }

                MonoType* fieldType = mono_field_get_type(iter);
                FieldType NotRedFieldType = GetFieldType(fieldType);

                if (NotRedFieldType == FieldType::ClassReference)
                {
                    continue;
                }

                MonoCustomAttrInfo* attr = mono_custom_attrs_from_field(scriptClass.Class, iter);

                char* typeName = mono_type_get_name(fieldType);

                if (oldFields.find(name) != oldFields.end())
                {
                    fieldMap.emplace(name, std::move(oldFields.at(name)));
                }
                else
                {
					PublicField field = { name, typeName, NotRedFieldType };
                    field.mEntityInstance = &entityInstance;
                    field.mMonoClassField = iter;

                    fieldMap.emplace(name, std::move(field));
                }
            }
        }
    }

    void ScriptEngine::ShutdownScriptEntity(Entity entity, const std::string& moduleName)
    {
        EntityInstanceData& entityInstanceData = GetEntityInstanceData(entity.GetSceneID(), entity.GetID());
        ScriptModuleFieldMap& moduleFieldMap = entityInstanceData.ModuleFieldMap;
        if (moduleFieldMap.find(moduleName) != moduleFieldMap.end())
        {
            moduleFieldMap.erase(moduleName);
        }
    }

    void ScriptEngine::InstantiateEntityClass(Entity entity)
    {
        Scene* scene = entity.mScene;
        UUID id = entity.GetComponent<IDComponent>().ID;
        auto& moduleName = entity.GetComponent<ScriptComponent>().ModuleName;

        EntityInstanceData& entityInstanceData = GetEntityInstanceData(scene->GetID(), id);
        EntityInstance& entityInstance = entityInstanceData.Instance;
        NR_CORE_ASSERT(entityInstance.ScriptClass);
        entityInstance.Handle = Instantiate(*entityInstance.ScriptClass);

        void* param[] = { &id };
		CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->Constructor, param);

        // Set all public fields to appropriate values
        ScriptModuleFieldMap& moduleFieldMap = entityInstanceData.ModuleFieldMap;
        if (moduleFieldMap.find(moduleName) != moduleFieldMap.end())
        {
            auto& publicFields = moduleFieldMap.at(moduleName);
            for (auto& [name, field] : publicFields)
                field.CopyStoredValueToRuntime();
        }

        // Call Create function (if exists)
        CreateEntity(entity);
    }

    EntityInstanceData& ScriptEngine::GetEntityInstanceData(UUID sceneID, UUID entityID)
    {
        NR_CORE_ASSERT(sEntityInstanceMap.find(sceneID) != sEntityInstanceMap.end(), "Invalid scene ID!");
        auto& entityIDMap = sEntityInstanceMap.at(sceneID);
        NR_CORE_ASSERT(entityIDMap.find(entityID) != entityIDMap.end(), "Invalid entity ID!");
        return entityIDMap.at(entityID);
    }

    EntityInstanceMap& ScriptEngine::GetEntityInstanceMap()
    {
        return sEntityInstanceMap;
    }

    static uint32_t GetFieldSize(FieldType type)
    {
        switch (type)
        {
        case FieldType::Float:       return 4;
        case FieldType::Int:         return 4;
        case FieldType::UnsignedInt: return 4;
            // case FieldType::String:   return 8; // TODO
        case FieldType::Vec2:        return 4 * 2;
        case FieldType::Vec3:        return 4 * 3;
        case FieldType::Vec4:        return 4 * 4;
        case FieldType::ClassReference: return 4;
        }
        NR_CORE_ASSERT(false, "Unknown field type!");
        return 0;
    }

    PublicField::PublicField(const std::string& name, const std::string& typeName, FieldType type)
        : Name(name), TypeName(typeName), Type(type)
    {
        mStoredValueBuffer = AllocateBuffer(type);
    }

    PublicField::PublicField(PublicField&& other)
    {
        Name = std::move(other.Name);
        TypeName = std::move(other.TypeName);
        Type = other.Type;
        mEntityInstance = other.mEntityInstance;
        mMonoClassField = other.mMonoClassField;
        mStoredValueBuffer = other.mStoredValueBuffer;

        other.mEntityInstance = nullptr;
        other.mMonoClassField = nullptr;
        other.mStoredValueBuffer = nullptr;
    }

    PublicField::~PublicField()
    {
        delete[] mStoredValueBuffer;
    }

    void PublicField::CopyStoredValueToRuntime()
    {
        NR_CORE_ASSERT(mEntityInstance->GetInstance());

        if (Type == FieldType::ClassReference)
        {
            void* params[] = {
                &mStoredValueBuffer
            };
            MonoObject* obj = ScriptEngine::Construct(TypeName + ":.ctor(intptr)", true, params);
            mono_field_set_value(mEntityInstance->GetInstance(), mMonoClassField, obj);
        }
        else
        {
            mono_field_set_value(mEntityInstance->GetInstance(), mMonoClassField, mStoredValueBuffer);
        }
    }

    bool PublicField::IsRuntimeAvailable() const
    {
        return mEntityInstance->Handle != 0;
    }

    void PublicField::SetStoredValueRaw(void* src)
    {
        if (Type == FieldType::ClassReference)
        {
            mStoredValueBuffer = (uint8_t*)src;
        }
        else
        {
            uint32_t size = GetFieldSize(Type);
            memcpy(mStoredValueBuffer, src, size);
        }
    }

    void PublicField::SetRuntimeValueRaw(void* src)
    {
        NR_CORE_ASSERT(mEntityInstance->GetInstance());
        mono_field_set_value(mEntityInstance->GetInstance(), mMonoClassField, src);
    }

    void* PublicField::GetRuntimeValueRaw()
    {
        NR_CORE_ASSERT(mEntityInstance->GetInstance());

        if (Type == FieldType::ClassReference)
        {
            MonoObject* instance;
            mono_field_get_value(mEntityInstance->GetInstance(), mMonoClassField, &instance);

            if (!instance)
            {
                return nullptr;
            }

            MonoClassField* field = mono_class_get_field_from_name(mono_object_get_class(instance), "_unmanagedInstance");
            int* value;
            mono_field_get_value(instance, field, &value);
            return value;
        }
        else
        {
            uint8_t* outValue;
            mono_field_get_value(mEntityInstance->GetInstance(), mMonoClassField, outValue);
            return outValue;
        }
    }

    uint8_t* PublicField::AllocateBuffer(FieldType type)
    {
        uint32_t size = GetFieldSize(type);
        uint8_t* buffer = new uint8_t[size];
        memset(buffer, 0, size);
        return buffer;
    }

    void PublicField::SetStoredValue_Internal(void* value) const
    {
        if (Type == FieldType::ClassReference)
        {
        }
        else
        {
            uint32_t size = GetFieldSize(Type);
            memcpy(mStoredValueBuffer, value, size);
        }
    }

    void PublicField::GetStoredValue_Internal(void* outValue) const
    {
        uint32_t size = GetFieldSize(Type);
        memcpy(outValue, mStoredValueBuffer, size);
    }

    void PublicField::SetRuntimeValue_Internal(void* value) const
    {
        NR_CORE_ASSERT(mEntityInstance->GetInstance());
        mono_field_set_value(mEntityInstance->GetInstance(), mMonoClassField, value);
    }

    void PublicField::GetRuntimeValue_Internal(void* outValue) const
    {
        NR_CORE_ASSERT(mEntityInstance->GetInstance());
        mono_field_get_value(mEntityInstance->GetInstance(), mMonoClassField, outValue);
    }

    void ScriptEngine::ImGuiRender()
    {
        ImGui::Begin("Script Engine Debug");
        for (auto& [sceneID, entityMap] : sEntityInstanceMap)
        {
            bool opened = ImGui::TreeNode((void*)(uint64_t)sceneID, "Scene (%llx)", sceneID);
            if (opened)
            {
                Ref<Scene> scene = Scene::GetScene(sceneID);
                for (auto& [entityID, entityInstanceData] : entityMap)
                {
                    Entity entity = scene->GetScene(sceneID)->GetEntityMap().at(entityID);
                    std::string entityName = entity.GetComponent<TagComponent>().Tag;

                    opened = ImGui::TreeNode((void*)(uint64_t)entityID, "%s (%llx)", entityName.c_str(), entityID);
                    if (opened)
                    {
                        for (auto& [moduleName, fieldMap] : entityInstanceData.ModuleFieldMap)
                        {
                            opened = ImGui::TreeNode(moduleName.c_str());
                            if (opened)
                            {
                                for (auto& [fieldName, field] : fieldMap)
                                {
                                    opened = ImGui::TreeNodeEx((void*)&field, ImGuiTreeNodeFlags_Leaf, fieldName.c_str());
                                    if (opened)
                                    {

                                        ImGui::TreePop();
                                    }
                                }
                                ImGui::TreePop();
                            }
                        }
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }
        }
        ImGui::End();
    }
}