#pragma once
#include <filesystem>
#include <string>

#include "NotRed/Scene/Scene.h"
#include "NotRed/Scene/Entity.h"
#include <map>

extern "C" 
{
	typedef struct _MonoClass MonoClass;
	typedef struct _MonoObject MonoObject;
	typedef struct _MonoMethod MonoMethod;
	typedef struct _MonoAssembly MonoAssembly;
	typedef struct _MonoImage MonoImage;
	typedef struct _MonoClassField MonoClassField;
}

namespace NR
{
	enum class ScriptFieldType
	{
		None,
		Char, Byte, Bool,
		Int, Float, Double, Long, Short,
		UByte, UInt, ULong, UShort,
		Vector2, Vector3, Vector4,
		Entity
	};

	struct ScriptField
	{
		ScriptFieldType Type;
		std::string Name;

		MonoClassField* ClassField;
	};

	class ScriptClass
	{
	public:
		ScriptClass() = default;
		ScriptClass(const std::string& classNamespace, const std::string& className, bool isCore = false);

		MonoObject* Instantiate();
		MonoObject* InvokeMethod(MonoObject* instance, MonoMethod* method, void** params = nullptr);
		MonoMethod* GetMethod(const std::string& name, int parameterCount);

		const std::map<std::string, ScriptField>& GetFields() const { return mFields; }

	private:
		std::string mClassNamespace;
		std::string mClassName;

		MonoClass* mMonoClass = nullptr;

		std::map<std::string, ScriptField> mFields;

		friend class ScriptEngine;
	};

	class ScriptInstance
	{
	public:
		ScriptInstance(Ref<ScriptClass> scriptClass, Entity entity);

		void InvokeCreate();
		void InvokeUpdate(float dt);

		Ref<ScriptClass> GetScriptClass() { return mScriptClass; }

		template<typename T>
		T GetFieldValue(const std::string& name)
		{
			bool success = GetFieldValueInternal(name, sFieldValueBuffer);
			if (!success)
			{
				return T();
			}

			return *(T*)sFieldValueBuffer;
		}

		template<typename T>
		void SetFieldValue(const std::string& name, const T& value)
		{
			SetFieldValueInternal(name, &value);
		}

	private:
		bool GetFieldValueInternal(const std::string& name, void* buffer);
		bool SetFieldValueInternal(const std::string& name, const void* value);

	private:
		Ref<ScriptClass> mScriptClass;

		MonoObject* mInstance = nullptr;
		MonoMethod* mConstructor = nullptr;
		MonoMethod* mCreateMethod = nullptr;
		MonoMethod* mUpdateMethod = nullptr;

		inline static char sFieldValueBuffer[8];
	};

	class ScriptEngine
	{
	public:
		static void Init();
		static void Shutdown();

		static void LoadAssembly(const std::filesystem::path& filepath);
		static void LoadAppAssembly(const std::filesystem::path& filepath);

		static void RuntimeStart(Scene* scene);
		static void RuntimeStop();

		static bool EntityClassExists(const std::string& className);
		static void CreateEntity(Entity entity);
		static void UpdateEntity(Entity entity, float dt);

		static Scene* GetSceneContext();
		static Ref<ScriptInstance> GetEntityScriptInstance(UUID entityID);
		static std::unordered_map<std::string, Ref<ScriptClass>> GetEntityClasses();

		static MonoImage* GetCoreAssemblyImage();

	private:
		static void InitMono();
		static void ShutdownMono();

		static MonoObject* InstantiateClass(MonoClass* monoClass);
		static void LoadAssemblyClasses();

		friend class ScriptClass;
		friend class ScriptGlue;
	};
}