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

		MonoClassField* ClassField = nullptr;
	};

	struct ScriptFieldInstance
	{
		ScriptField Field;

		ScriptFieldInstance()
		{
			memset(mBuffer, 0, sizeof(mBuffer));
		}

		template<typename T>
		T GetValue()
		{
			NR_ASSERT(sizeof(T) <= 16, "Type too large!");
			return *(T*)mBuffer;
		}

		template<typename T>
		void SetValue(T value)
		{
			NR_ASSERT(sizeof(T) <= 16, "Type too large!");
			memcpy(mBuffer, &value, sizeof(T));
		}
	private:
		uint8_t mBuffer[16];

		friend class ScriptEngine;
		friend class ScriptInstance;
	};

	using ScriptFieldMap = std::unordered_map<std::string, ScriptFieldInstance>;

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

	class ScriptClassInstance
	{
	public:
		ScriptClassInstance(Ref<ScriptClass> scriptClass, Entity entity);

		void InvokeCreate();
		void InvokeUpdate(float dt);

		Ref<ScriptClass> GetScriptClass() { return mScriptClass; }

		template<typename T>
		T GetFieldValue(const std::string& name)
		{
			NR_ASSERT(sizeof(T) <= 16, "Type too large!");
			bool success = GetFieldValueInternal(name, sFieldValueBuffer);
			if (!success)
			{
				return T();
			}

			return *(T*)sFieldValueBuffer;
		}

		template<typename T>
		void SetFieldValue(const std::string& name, T value)
		{
			NR_ASSERT(sizeof(T) <= 16, "Type too large!");
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

		inline static char sFieldValueBuffer[16];

		friend class ScriptEngine;
		friend struct ScriptFieldInstance;
	};

	class ScriptEngine
	{
	public:
		static void Init();
		static void Shutdown();

		static bool LoadAssembly(const std::filesystem::path& filepath);
		static bool LoadAppAssembly(const std::filesystem::path& filepath);

		static void ReloadAssembly();

		static void RuntimeStart(Scene* scene);
		static void RuntimeStop();

		static bool EntityClassExists(const std::string& className);
		static void ConstructEntity(Entity entity);
		static void UpdateEntity(Entity entity, float dt);

		static Ref<ScriptClass> GetEntityClass(const std::string& name);
		static Scene* GetSceneContext();
		static Ref<ScriptClassInstance> GetEntityScriptInstance(UUID entityID);

		static std::unordered_map<std::string, Ref<ScriptClass>> GetEntityClasses();
		static ScriptFieldMap& GetScriptFieldMap(Entity entity);

		static MonoImage* GetCoreAssemblyImage();

	private:
		static void InitMono();
		static void ShutdownMono();

		static MonoObject* InstantiateClass(MonoClass* monoClass);
		static void LoadAssemblyClasses();

		friend class ScriptClass;
		friend class ScriptGlue;
	};

	namespace Utils
	{
		inline const char* ScriptFieldTypeToString(ScriptFieldType type)
		{
			switch (type)
			{
			case ScriptFieldType::Char:     return "Char";
			case ScriptFieldType::Byte:     return "Byte";
			case ScriptFieldType::Bool:     return "Bool";

			case ScriptFieldType::Int:      return "Int";
			case ScriptFieldType::Float:    return "Float";
			case ScriptFieldType::Double:   return "Double";
			case ScriptFieldType::Long:     return "Long";
			case ScriptFieldType::Short:    return "Short";

			case ScriptFieldType::UByte:    return "UByte";
			case ScriptFieldType::UInt:     return "UInt";
			case ScriptFieldType::ULong:    return "ULong";
			case ScriptFieldType::UShort:   return "UShort";

			case ScriptFieldType::Vector2:  return "Vector2";
			case ScriptFieldType::Vector3:  return "Vector3";
			case ScriptFieldType::Vector4:  return "Vector4";

			case ScriptFieldType::Entity:   return "Entity";

			case ScriptFieldType::None:
			default: return "<Unknown>";
			}
		}

		inline ScriptFieldType StringtoScriptFieldType(std::string_view fieldType)
		{
				 if (fieldType == "Char")     return ScriptFieldType::Char;
			else if (fieldType == "Byte")     return ScriptFieldType::Byte;
			else if (fieldType == "Bool")     return ScriptFieldType::Bool;
										 
			else if (fieldType == "Int")      return ScriptFieldType::Int;
			else if (fieldType == "Float")    return ScriptFieldType::Float;
			else if (fieldType == "Double")   return ScriptFieldType::Double;
			else if (fieldType == "Long")     return ScriptFieldType::Long;
			else if (fieldType == "Short")    return ScriptFieldType::Short;

			else if (fieldType == "UByte")    return ScriptFieldType::UByte;
			else if (fieldType == "UInt")     return ScriptFieldType::UInt;
			else if (fieldType == "ULong")    return ScriptFieldType::ULong;
			else if (fieldType == "UShort")   return ScriptFieldType::UShort;

			else if (fieldType == "Vector2")  return ScriptFieldType::Vector2;
			else if (fieldType == "Vector3")  return ScriptFieldType::Vector3;
			else if (fieldType == "Vector4")  return ScriptFieldType::Vector4;

			else if (fieldType == "Entity")   return ScriptFieldType::Entity;

			else if (fieldType == "None")     return ScriptFieldType::None;

			NR_CORE_ASSERT(false, "Unknown ScriptFieldType");
			return ScriptFieldType::None;
		}
	}
}