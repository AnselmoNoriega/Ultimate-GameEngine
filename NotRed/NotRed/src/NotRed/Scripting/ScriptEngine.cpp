#include "nrpch.h"
#include "ScriptEngine.h"

#include "mono/jit/jit.h"
#include "mono/metadata/assembly.h"
#include "mono/metadata/object.h"

namespace NR
{
	struct ScriptEngineData
	{
		MonoDomain* RootDomain = nullptr;
		MonoDomain* AppDomain = nullptr;

		MonoAssembly* CoreAssembly = nullptr;
	};

	static ScriptEngineData* sData = nullptr;

    void ScriptEngine::Init()
    {
		sData = new ScriptEngineData();
		InitMono();
    }

    void ScriptEngine::Shutdown()
    {
		ShutdownMono();
		delete sData;
    }

	char* ReadBytes(const std::string& filepath, uint32_t* outSize)
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

	MonoAssembly* LoadCSharpAssembly(const std::string& assemblyPath)
	{
		uint32_t fileSize = 0;
		char* fileData = ReadBytes(assemblyPath, &fileSize);

		// NOTE: We can't use this image for anything other than loading the assembly because this image doesn't have a reference to the assembly
		MonoImageOpenStatus status;
		MonoImage* image = mono_image_open_from_data_full(fileData, fileSize, 1, &status, 0);

		if (status != MONO_IMAGE_OK)
		{
			const char* errorMessage = mono_image_strerror(status);
			NR_CORE_ERROR(errorMessage);
			return nullptr;
		}

		MonoAssembly* assembly = mono_assembly_load_from_full(image, assemblyPath.c_str(), &status, 0);
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

    void ScriptEngine::InitMono()
    {
        mono_set_assemblies_path("mono/lib");

		MonoDomain* rootDomain = mono_jit_init("NotJITRuntime");
		NR_CORE_ASSERT(rootDomain, "Missing root domain");

		sData->RootDomain = rootDomain;

		sData->AppDomain = mono_domain_create_appdomain((char*)"NotScriptRuntime", nullptr);
		mono_domain_set(sData->AppDomain, true);

		sData->CoreAssembly = LoadCSharpAssembly("Resources/Scripts/NotRed-ScriptCore.dll");
		PrintAssemblyTypes(sData->CoreAssembly);

		MonoImage* assemblyImage = mono_assembly_get_image(sData->CoreAssembly);
		MonoClass* monoClass = mono_class_from_name(assemblyImage, "NR", "Main");

		MonoObject* instance = mono_object_new(sData->AppDomain, monoClass);
		mono_runtime_object_init(instance);

		MonoMethod* func = mono_class_get_method_from_name(monoClass, "PrintMessage", 0);
		mono_runtime_invoke(func, instance, nullptr, nullptr);

		MonoMethod* paramFunc = mono_class_get_method_from_name(monoClass, "PrintInt", 1);

		int value = 5;
		void* param = &value;

		mono_runtime_invoke(paramFunc, instance, &param, nullptr);

		MonoMethod* paramsFunc = mono_class_get_method_from_name(monoClass, "PrintInts", 2);
		int value2 = 508;
		void* params[2] =
		{
			&value,
			&value2
		};
		mono_runtime_invoke(paramsFunc, instance, params, nullptr);

		MonoString* monoString = mono_string_new(sData->AppDomain, "Hello World from C++!");
		MonoMethod* printCustomMessageFunc = mono_class_get_method_from_name(monoClass, "PrintCustomMessage", 1);
		void* stringParam = monoString;
		mono_runtime_invoke(printCustomMessageFunc, instance, &stringParam, nullptr);
    }

	void ScriptEngine::ShutdownMono()
	{
		sData->AppDomain = nullptr;
		sData->RootDomain = nullptr;
	}
}