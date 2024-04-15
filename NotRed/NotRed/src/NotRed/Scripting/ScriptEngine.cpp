#include "nrpch.h"
#include "ScriptEngine.h"

#include "ScriptGlue.h"

#include "mono/jit/jit.h"
#include "mono/metadata/assembly.h"
#include "mono/metadata/object.h"

namespace NR
{
    namespace Utils
    {
        static char* ReadBytes(const std::string& filepath, uint32_t* outSize)
        {
            std::ifstream stream(filepath, std::ios::binary | std::ios::ate);

            if (!stream)
            {
                // Failed to open the file
                return nullptr;
            }

            std::streampos end = stream.tellg();
            stream.seekg(0, std::ios::beg);
            uint32_t size = end - stream.tellg();

            if (size == 0)
            {
                // File is empty
                return nullptr;
            }

            char* buffer = new char[size];
            stream.read((char*)buffer, size);
            stream.close();

            *outSize = size;
            return buffer;
        }

        static MonoAssembly* LoadMonoAssembly(const std::filesystem::path& assemblyPath)
        {
            uint32_t fileSize = 0;
            char* fileData = ReadBytes(assemblyPath.string(), &fileSize);

            // NOTE: We can't use this image for anything other than loading the assembly because this image doesn't have a reference to the assembly
            MonoImageOpenStatus status;
            MonoImage* image = mono_image_open_from_data_full(fileData, fileSize, 1, &status, 0);

            if (status != MONO_IMAGE_OK)
            {
                const char* errorMessage = mono_image_strerror(status);
                NR_CORE_ERROR(errorMessage);
                return nullptr;
            }

            std::string pathString = assemblyPath.string();
            MonoAssembly* assembly = mono_assembly_load_from_full(image, pathString.c_str(), &status, 0);
            mono_image_close(image);

            // Don't forget to free the file data
            delete[] fileData;

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
    }

    struct ScriptEngineData
    {
        MonoDomain* RootDomain = nullptr;
        MonoDomain* AppDomain = nullptr;

        MonoAssembly* CoreAssembly = nullptr;
        MonoImage* CoreAssemblyImage = nullptr;

        ScriptClass EntityClass;
    };

    static ScriptEngineData* sMonoData = nullptr;

    void ScriptEngine::Init()
    {
        sMonoData = new ScriptEngineData();

        InitMono();
        LoadAssembly("Resources/Scripts/NotRed-ScriptCore.dll");

        ScriptGlue::RegisterFunctions();

        sMonoData->EntityClass = ScriptClass("NR", "Entity");

        MonoObject* instance = sMonoData->EntityClass.Instantiate();

        MonoMethod* method = sMonoData->EntityClass.GetMethod("PrintMessage", 0);
        sMonoData->EntityClass.InvokeMethod(instance, method);

        MonoMethod* methodParam = sMonoData->EntityClass.GetMethod("PrintInt", 1);
        int value = 5;
        void* param = &value;
        sMonoData->EntityClass.InvokeMethod(instance, methodParam, &param);

        MonoMethod* methodParams = sMonoData->EntityClass.GetMethod("PrintInts", 2);
        int value2 = 508;
        void* params[2] =
        {
            &value,
            &value2
        };
        sMonoData->EntityClass.InvokeMethod(instance, methodParams, params);

        MonoString* monoString = mono_string_new(sMonoData->AppDomain, "Hello World from C++!");
        MonoMethod* methodString = sMonoData->EntityClass.GetMethod("PrintCustomMessage", 1);
        void* stringParam = monoString;
        sMonoData->EntityClass.InvokeMethod(instance, methodString, &stringParam);
    }

    void ScriptEngine::InitMono()
    {
        mono_set_assemblies_path("mono/lib");

        MonoDomain* rootDomain = mono_jit_init("NotJITRuntime");
        NR_CORE_ASSERT(rootDomain, "Unable to Initialize root Domain!");

        sMonoData->RootDomain = rootDomain;
    }

    MonoObject* ScriptEngine::InstantiateClass(MonoClass* monoClass)
    {
        MonoObject* instance = mono_object_new(sMonoData->AppDomain, monoClass);
        mono_runtime_object_init(instance);
        return instance;
    }

    void ScriptEngine::Shutdown()
    {
        ShutdownMono();
        delete sMonoData;
    }

    void ScriptEngine::ShutdownMono()
    {
        sMonoData->AppDomain = nullptr;
        sMonoData->RootDomain = nullptr;
    }

    void ScriptEngine::LoadAssembly(const std::filesystem::path& filepath)
    {
        sMonoData->AppDomain = mono_domain_create_appdomain((char*)"NotScriptRuntime", nullptr);
        mono_domain_set(sMonoData->AppDomain, true);

        sMonoData->CoreAssembly = Utils::LoadMonoAssembly(filepath);
        sMonoData->CoreAssemblyImage = mono_assembly_get_image(sMonoData->CoreAssembly);
    }

    ScriptClass::ScriptClass(const std::string& classNamespace, const std::string& className)
        : mClassNamespace(classNamespace), mClassName(className)
    {
        mMonoClass = mono_class_from_name(sMonoData->CoreAssemblyImage, classNamespace.c_str(), className.c_str());
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
        return mono_runtime_invoke(method, instance, params, nullptr);
    }
}