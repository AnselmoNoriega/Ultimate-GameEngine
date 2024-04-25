#include "nrpch.h"
#include "ScriptEngine.h"

#include "ScriptGlue.h"

#include "mono/jit/jit.h"
#include "mono/metadata/object.h"
#include "mono/metadata/assembly.h"
#include "mono/metadata/attrdefs.h"
#include "mono/metadata/mono-debug.h"
#include "mono/metadata/threads.h"

#include "NotRed/Project/Project.h"

#include "FileWatch.h"
#include "NotRed/Core/Buffer.h"
#include "NotRed/Core/FileSystem.h"

#include "NotRed/Core/Application.h"

namespace NR
{
    static std::unordered_map<std::string, ScriptFieldType> sScriptFieldTypeMap =
    {
        { "Char",    ScriptFieldType::Char    },
        { "Byte",    ScriptFieldType::Byte    },
        { "Boolean", ScriptFieldType::Bool    },

        { "Int32",   ScriptFieldType::Int     },
        { "Single",  ScriptFieldType::Float   },
        { "Double",  ScriptFieldType::Double  },
        { "Int64",   ScriptFieldType::Long    },
        { "Int16",   ScriptFieldType::Short   },

        { "UByte",   ScriptFieldType::UByte   },
        { "UInt32",  ScriptFieldType::UInt    },
        { "UInt64",  ScriptFieldType::ULong   },
        { "UInt16",  ScriptFieldType::UShort  },

        { "Vector2", ScriptFieldType::Vector2 },
        { "Vector3", ScriptFieldType::Vector3 },
        { "Vector4", ScriptFieldType::Vector4 },

        { "Entity",  ScriptFieldType::Entity  }
    };

    namespace Utils
    {
        static MonoAssembly* LoadMonoAssembly(const std::filesystem::path& assemblyPath, bool loadPDB = false)
        {
            ScopedBuffer fileData = FileSystem::ReadFileBinary<char>(assemblyPath);

            MonoImageOpenStatus status;
            MonoImage* image = mono_image_open_from_data_full(fileData.Data(), fileData.Size(), 1, &status, 0);

            if (status != MONO_IMAGE_OK)
            {
                const char* errorMessage = mono_image_strerror(status);
                NR_CORE_ERROR(errorMessage);
                return nullptr;
            }

            if (loadPDB)
            {
                std::filesystem::path pdbPath = assemblyPath;
                pdbPath.replace_extension(".pdb");

                if (std::filesystem::exists(pdbPath))
                {
                    ScopedBuffer pdbFile = FileSystem::ReadFileBinary<mono_byte>(pdbPath);
                    mono_debug_open_image_from_memory(image, pdbFile.Data(), pdbFile.Size());
                    NR_CORE_INFO("Loaded PDB {}", pdbPath);
                }
            }

            std::string pathString = assemblyPath.string();
            MonoAssembly* assembly = mono_assembly_load_from_full(image, pathString.c_str(), &status, 0);
            mono_image_close(image);

            return assembly;
        }

        void PrintAssemblyTypes(MonoAssembly* assembly)
        {
            MonoImage* image = mono_assembly_get_image(assembly);
            const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
            int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);

            for (int32_t i = 0; i < numTypes; i++)
            {
                uint32_t cols[MONO_TYPEDEF_SIZE];
                mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

                const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
                const char* name = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);

                NR_CORE_TRACE("{}.{}", nameSpace, name);
            }
        }

        ScriptFieldType MonoTypeToScriptFieldType(MonoType* monoType)
        {
            std::string typeName = mono_type_get_name(monoType);

            size_t pos = typeName.find_last_of('.');
            typeName = typeName.substr(pos + 1);

            auto it = sScriptFieldTypeMap.find(typeName);
            if (it != sScriptFieldTypeMap.end())
            {
                return it->second;
            }

            NR_CORE_ERROR("Unknown type: {}", typeName);
            return ScriptFieldType::None;
        }
    }

    struct ScriptEngineData
    {
        MonoDomain* RootDomain = nullptr;
        MonoDomain* AppDomain = nullptr;

        MonoAssembly* CoreAssembly = nullptr;
        MonoImage* CoreAssemblyImage = nullptr;

        MonoAssembly* AppAssembly = nullptr;
        MonoImage* AppAssemblyImage = nullptr;

        std::filesystem::path CoreAssemblyFilepath;
        std::filesystem::path AppAssemblyFilepath;

        ScriptClass EntityClass;

        std::unordered_map<std::string, Ref<ScriptClass>> EntityClasses;
        std::unordered_map<UUID, Ref<ScriptClassInstance>> EntityInstances;
        std::unordered_map<UUID, ScriptFieldMap> EntityScriptFields;

        Scope<filewatch::FileWatch<std::string>> AppAssemblyFileWatcher;
        bool AssemblyReloadPending = false;

#ifdef NR_DEBUG
        bool DebugEnabled = true;
#else
        bool DebugEnabled = false;
#endif

        Scene* SceneContext = nullptr;
    };

    static ScriptEngineData* sMonoData = nullptr;

    static void AssemblyFileSystemEvent(const std::string& path, const filewatch::Event changeType)
    {
        if (!sMonoData->AssemblyReloadPending && changeType == filewatch::Event::modified)
        {
            sMonoData->AssemblyReloadPending = true;

            Application::Get().SubmitToMainThread([]()
                {
                    sMonoData->AppAssemblyFileWatcher.reset();
                    ScriptEngine::ReloadAssembly();
                });
        }
    }

    void ScriptEngine::InitMono()
    {
        mono_set_assemblies_path("mono/lib");

        if (sMonoData->DebugEnabled)
        {
            const char* argv[2] = {
                "--debugger-agent=transport=dt_socket,address=127.0.0.1:2550,server=y,suspend=n,loglevel=3,logfile=NR_MonoDebugger.log",
                "--soft-breakpoints"
            };

            mono_jit_parse_options(2, (char**)argv);
            mono_debug_init(MONO_DEBUG_FORMAT_MONO);
        }

        MonoDomain* rootDomain = mono_jit_init("NotJITRuntime");
        NR_CORE_ASSERT(rootDomain, "Unable to Initialize root Domain!");

        sMonoData->RootDomain = rootDomain;

        if (sMonoData->DebugEnabled)
        {
            mono_debug_domain_create(sMonoData->RootDomain);
        }

        mono_thread_set_main(mono_thread_current());
    }

    MonoObject* ScriptEngine::InstantiateClass(MonoClass* monoClass)
    {
        MonoObject* instance = mono_object_new(sMonoData->AppDomain, monoClass);
        mono_runtime_object_init(instance);
        return instance;
    }

    bool ScriptEngine::LoadAssembly(const std::filesystem::path& filepath)
    {
        sMonoData->AppDomain = mono_domain_create_appdomain((char*)"NotScriptRuntime", nullptr);
        mono_domain_set(sMonoData->AppDomain, true);

        sMonoData->CoreAssemblyFilepath = filepath;
        sMonoData->CoreAssembly = Utils::LoadMonoAssembly(filepath, sMonoData->DebugEnabled);
        if (sMonoData->CoreAssembly == nullptr)
        {
            return false;
        }

        sMonoData->CoreAssemblyImage = mono_assembly_get_image(sMonoData->CoreAssembly);
        
        return true;
    }

    bool ScriptEngine::LoadAppAssembly(const std::filesystem::path& filepath)
    {
        sMonoData->AppAssemblyFilepath = filepath;
        sMonoData->AppAssembly = Utils::LoadMonoAssembly(filepath, sMonoData->DebugEnabled);
        if (sMonoData->AppAssembly == nullptr)
        {
            return false;
        }
        
        sMonoData->AppAssemblyImage = mono_assembly_get_image(sMonoData->AppAssembly);

        sMonoData->AppAssemblyFileWatcher = CreateScope<filewatch::FileWatch<std::string>>(filepath.string(), AssemblyFileSystemEvent);
        sMonoData->AssemblyReloadPending = false;
        
        return true;
    }

    void ScriptEngine::LoadAssemblyClasses()
    {
        sMonoData->EntityClasses.clear();

        const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(sMonoData->AppAssemblyImage, MONO_TABLE_TYPEDEF);
        int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);
        MonoClass* entityClass = mono_class_from_name(sMonoData->CoreAssemblyImage, "NotRed", "Entity");

        for (int32_t i = 0; i < numTypes; i++)
        {
            uint32_t cols[MONO_TYPEDEF_SIZE];
            mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

            const char* nameSpace = mono_metadata_string_heap(sMonoData->AppAssemblyImage, cols[MONO_TYPEDEF_NAMESPACE]);
            const char* className = mono_metadata_string_heap(sMonoData->AppAssemblyImage, cols[MONO_TYPEDEF_NAME]);
            std::string strName = className;

            MonoClass* monoClass = mono_class_from_name(sMonoData->AppAssemblyImage, nameSpace, className);

            if (monoClass == entityClass)
            {
                continue;
            }

            bool isEntity = mono_class_is_subclass_of(monoClass, entityClass, false);
            if (!isEntity)
            {
                continue;
            }

            Ref<ScriptClass> scriptClass = CreateRef<ScriptClass>(nameSpace, className);
            sMonoData->EntityClasses[className] = scriptClass;

            int fieldCount = mono_class_num_fields(monoClass);
            NR_CORE_INFO("{} has {} fields:", className, fieldCount);

            void* iterator = nullptr;
            while (MonoClassField* field = mono_class_get_fields(monoClass, &iterator))
            {
                const char* fieldName = mono_field_get_name(field);
                uint32_t flags = mono_field_get_flags(field);
                if (flags & MONO_FIELD_ATTR_PUBLIC)
                {
                    MonoType* monoType = mono_field_get_type(field);
                    ScriptFieldType fieldType = Utils::MonoTypeToScriptFieldType(monoType);
                    NR_CORE_INFO("{} ({})", fieldName, Utils::ScriptFieldTypeToString(fieldType));

                    scriptClass->mFields[fieldName] = { fieldType, fieldName, field };
                }
            }
        }
    }

    void ScriptEngine::ReloadAssembly()
    {
        mono_domain_set(mono_get_root_domain(), false);

        mono_domain_unload(sMonoData->AppDomain);

        if (!LoadAssembly(sMonoData->CoreAssemblyFilepath))
        {
            NR_CORE_ERROR("Failed to load ScriptCore assembly!");
        }
        if (!LoadAppAssembly(sMonoData->AppAssemblyFilepath))
        {
            NR_CORE_ERROR("Failed to load app assembly!");
        }
        LoadAssemblyClasses();

        ScriptGlue::RegisterComponents();

        sMonoData->EntityClass = ScriptClass("NotRed", "Entity", true);
    }

    Ref<ScriptClassInstance> ScriptEngine::GetEntityScriptInstance(UUID entityID)
    {
        auto it = sMonoData->EntityInstances.find(entityID);
        if (it != sMonoData->EntityInstances.end())
        {
            return it->second;
        }

        return nullptr;
    }

    void ScriptEngine::Init()
    {
        sMonoData = new ScriptEngineData();

        InitMono();
        ScriptGlue::RegisterFunctions();

        if (!LoadAssembly("Resources/Scripts/NotRed-ScriptCore.dll"))
        {
            NR_CORE_ERROR("Failed to load ScriptCore assembly!");
        }

        auto scriptModulePath = Project::GetAssetDirectory() / Project::GetActive()->GetConfig().ScriptModulePath;
        if (!LoadAppAssembly(scriptModulePath))
        {
            NR_CORE_ERROR("Failed to load app assembly!");
        }
        LoadAssemblyClasses();

        ScriptGlue::RegisterComponents();

        sMonoData->EntityClass = ScriptClass("NotRed", "Entity", true);

        MonoObject* instance = sMonoData->EntityClass.Instantiate();
    }

    void ScriptEngine::RuntimeStart(Scene* scene)
    {
        sMonoData->SceneContext = scene;
    }

    bool ScriptEngine::EntityClassExists(const std::string& className)
    {
        return sMonoData->EntityClasses.find(className) != sMonoData->EntityClasses.end();
    }

    void ScriptEngine::ConstructEntity(Entity entity)
    {
        const auto& sc = entity.GetComponent<ScriptComponent>();
        if (ScriptEngine::EntityClassExists(sc.ClassName))
        {
            UUID entityID = entity.GetUUID();

            Ref<ScriptClassInstance> instance = CreateRef<ScriptClassInstance>(sMonoData->EntityClasses[sc.ClassName], entity);
            sMonoData->EntityInstances[entityID] = instance;

            if (sMonoData->EntityScriptFields.find(entityID) != sMonoData->EntityScriptFields.end())
            {
                const ScriptFieldMap& fieldMap = sMonoData->EntityScriptFields.at(entityID);
                for (const auto& [name, fieldInstance] : fieldMap)
                {
                    instance->SetFieldValueInternal(name, fieldInstance.mBuffer);
                }
            }

            instance->InvokeCreate();
        }
    }

    void ScriptEngine::UpdateEntity(Entity entity, float dt)
    {
        UUID entityUUID = entity.GetUUID();

        if(sMonoData->EntityInstances.find(entityUUID) != sMonoData->EntityInstances.end())
        {
            Ref<ScriptClassInstance> instance = sMonoData->EntityInstances[entityUUID];
            instance->InvokeUpdate(dt);
        }
        else
        {
            NR_CORE_ERROR("Entity ({}) has no Instance", entityUUID);
        }
    }

    void ScriptEngine::RuntimeStop()
    {
        sMonoData->SceneContext = nullptr;
        sMonoData->EntityInstances.clear();
    }

    void ScriptEngine::ShutdownMono()
    {
        mono_domain_set(mono_get_root_domain(), false);

        mono_domain_unload(sMonoData->AppDomain);
        sMonoData->AppDomain = nullptr;

        mono_jit_cleanup(sMonoData->RootDomain);
        sMonoData->RootDomain = nullptr;
    }

    void ScriptEngine::Shutdown()
    {
        ShutdownMono();
        delete sMonoData;
    }

    std::unordered_map<std::string, Ref<ScriptClass>> ScriptEngine::GetEntityClasses()
    {
        return sMonoData->EntityClasses;
    }

    ScriptFieldMap& ScriptEngine::GetScriptFieldMap(Entity entity)
    {
        return sMonoData->EntityScriptFields[entity.GetUUID()];
    }

    Scene* ScriptEngine::GetSceneContext()
    {
        return sMonoData->SceneContext;
    }

    MonoImage* ScriptEngine::GetCoreAssemblyImage()
    {
        return sMonoData->CoreAssemblyImage;
    }

    Ref<ScriptClass> ScriptEngine::GetEntityClass(const std::string& name)
    {
        if (sMonoData->EntityClasses.find(name) != sMonoData->EntityClasses.end())
        {
            return sMonoData->EntityClasses.at(name);
        }

        return nullptr;
    }

    ScriptClassInstance::ScriptClassInstance(Ref<ScriptClass> scriptClass, Entity entity)
        : mScriptClass(scriptClass)
    {
        mInstance = scriptClass->Instantiate();

        mConstructor = sMonoData->EntityClass.GetMethod(".ctor", 1);
        mCreateMethod = scriptClass->GetMethod("Create", 0);
        mUpdateMethod = scriptClass->GetMethod("Update", 1);

        UUID entityID = entity.GetUUID();
        void* param = &entityID;
        mScriptClass->InvokeMethod(mInstance, mConstructor, &param);
    }

    void ScriptClassInstance::InvokeCreate()
    {
        if (mCreateMethod)
        {
            mScriptClass->InvokeMethod(mInstance, mCreateMethod);
        }
    }

    void ScriptClassInstance::InvokeUpdate(float dt)
    {
        if (mUpdateMethod)
        {
            void* param = &dt;
            mScriptClass->InvokeMethod(mInstance, mUpdateMethod, &param);
        }
    }

    bool ScriptClassInstance::GetFieldValueInternal(const std::string& name, void* buffer)
    {
        const auto& fields = mScriptClass->GetFields();
        auto it = fields.find(name);
        if (it != fields.end())
        {
            const ScriptField& field = it->second;
            mono_field_get_value(mInstance, field.ClassField, buffer);
            return true;
        }

        return false;
    }

    bool ScriptClassInstance::SetFieldValueInternal(const std::string& name, const void* value)
    {
        const auto& fields = mScriptClass->GetFields();
        auto it = fields.find(name);
        if (it != fields.end())
        {
            const ScriptField& field = it->second;
            mono_field_set_value(mInstance, field.ClassField, (void*)value);
            return true;
        }

        return false;
    }

    ScriptClass::ScriptClass(const std::string& classNamespace, const std::string& className, bool isCore)
        : mClassNamespace(classNamespace), mClassName(className)
    {
        mMonoClass = mono_class_from_name(isCore ? sMonoData->CoreAssemblyImage : sMonoData->AppAssemblyImage,
            classNamespace.c_str(), className.c_str());
    }

    MonoObject* ScriptClass::Instantiate()
    {
        return ScriptEngine::InstantiateClass(mMonoClass);
    }

    MonoMethod* ScriptClass::GetMethod(const std::string& name, int parameterCount)
    {
        return mono_class_get_method_from_name(mMonoClass, name.c_str(), parameterCount);
    }

    MonoObject* ScriptClass::InvokeMethod(MonoObject* instance, MonoMethod* method, void** params)
    {
        MonoObject* exc = nullptr;
        return mono_runtime_invoke(method, instance, params, &exc);
    }
}