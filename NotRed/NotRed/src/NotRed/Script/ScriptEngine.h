#pragma once

#include "NotRed/Core/Core.h"
#include "NotRed/Core/TimeFrame.h"

#include <string>

#include "NotRed/Scene/Components.h"
#include "NotRed/Scene/Entity.h"

#include "ScriptModuleField.h"

extern "C" {
	typedef struct _MonoObject MonoObject;
	typedef struct _MonoClassField MonoClassField;
	typedef struct _MonoProperty MonoProperty;
	typedef struct _MonoClass MonoClass;
}

namespace NR
{
	enum class FieldType
	{
		None,
		Int, Float, UnsignedInt,
		String,
		Vec2, Vec3, Vec4,
		ClassReference, Asset
	};

	const char* FieldTypeToString(FieldType type);

	struct EntityScriptClass;
	struct EntityInstance
	{
		EntityScriptClass* ScriptClass = nullptr;

		uint32_t Handle = 0;
		Scene* SceneInstance = nullptr;

		MonoObject* GetInstance();
	};

	struct PublicField
	{
		std::string Name;
		std::string TypeName;
		FieldType Type;
		bool IsReadOnly;

		PublicField(const std::string& name, const std::string& typeName, FieldType type, bool isReadOnly = false);
		PublicField(const PublicField&) = delete;
		PublicField(PublicField&& other);
		~PublicField();

		void CopyStoredValueToRuntime();
		void CopyStoredValueFromRuntime();
		bool IsRuntimeAvailable() const;

		template<typename T>
		T GetStoredValue() const
		{
			T value;
			GetStoredValue_Internal(&value);
			return value;
		}

		template<>
		const std::string& GetStoredValue() const
		{
			return *(std::string*)mStoredValueBuffer;
		}

		template<typename T>
		void SetStoredValue(T value)
		{
			SetStoredValue_Internal(&value);
		}

		template<>
		void SetStoredValue(const std::string& value)
		{
			(*(std::string*)mStoredValueBuffer).assign(value);
		}

		template<typename T>
		T GetValue()
		{
			T value;
			GetRuntimeValue_Internal(&value);
			return value;
		}

		template<>
		std::string GetValue()
		{
			std::string value;
			GetRuntimeValue_Internal((void*)&value);
			return value;
		}

		template<typename T>
		void SetValue(T value)
		{
			SetRuntimeValue_Internal(&value);
		}

		template<>
		void SetValue(const std::string& value)
		{
			SetRuntimeValue_Internal((void*)&value);
		}

		void SetStoredValueRaw(void* src);
		void* GetStoredValueRaw() { return mStoredValueBuffer; }

		void SetRuntimeValueRaw(void* src);
		void* GetRuntimeValueRaw();

	private:
		uint8_t* AllocateBuffer(FieldType type);
		void SetStoredValue_Internal(void* value) const;
		void GetStoredValue_Internal(void* outValue) const;
		void SetRuntimeValue_Internal(void* value);
		void SetRuntimeValue_Internal(const std::string& value);
		void GetRuntimeValue_Internal(void* outValue);
		void GetRuntimeValue_Internal(std::string& outValue);

	private:
		EntityInstance* mEntityInstance = nullptr;
		MonoClassField* mMonoClassField = nullptr;
		MonoProperty* mMonoProperty = nullptr;
		uint8_t* mStoredValueBuffer = nullptr;

		friend class ScriptEngine;
	};

	using ScriptModuleFieldMap = std::unordered_map<std::string, std::unordered_map<std::string, PublicField>>;

	struct EntityInstanceData
	{
		EntityInstance Instance;
		ScriptModuleFieldMap ModuleFieldMap;
	};

	using EntityInstanceMap = std::unordered_map<UUID, std::unordered_map<UUID, EntityInstanceData>>;

	class ScriptEngine
	{
	public:
		static void Init(const std::string& assemblyPath);
		static void Shutdown();

		static void SceneDestruct(UUID sceneID);

		static bool LoadRuntimeAssembly(const std::string& path);
		static bool LoadAppAssembly(const std::string& path);
		static bool ReloadAssembly(const std::string& path);

		static void SetSceneContext(const Ref<Scene>& scene);
		static const Ref<Scene>& GetCurrentSceneContext();

		static void CopyEntityScriptData(UUID dst, UUID src);

		static void CreateEntity(Entity entity);
		static void UpdateEntity(Entity entity, float ts);
		static void UpdatePhysicsEntity(Entity entity, float fixedDeltaTime);

		static void Collision2DBegin(Entity entity, Entity other);
		static void Collision2DEnd(Entity entity, Entity other);

		static void CollisionBegin(Entity entity, Entity other);
		static void CollisionEnd(Entity entity, Entity other);

		static void TriggerBegin(Entity entity, Entity other);
		static void TriggerEnd(Entity entity, Entity other);

		static MonoObject* Construct(const std::string& fullName, bool callConstructor = true, void** parameters = nullptr);
		static MonoClass* GetCoreClass(const std::string& fullName);

		static bool IsEntityModuleValid(Entity entity);

		static void ScriptComponentDestroyed(UUID sceneID, UUID entityID);

		static std::string StripNamespace(const std::string& nameSpace, const std::string& moduleName);

		static bool ModuleExists(const std::string& moduleName);
		static void InitScriptEntity(Entity entity);
		static void ShutdownScriptEntity(Entity entity, const std::string& moduleName);
		static void InstantiateEntityClass(Entity entity);

		static EntityInstanceMap& GetEntityInstanceMap();
		static EntityInstanceData& GetEntityInstanceData(UUID sceneID, UUID entityID);

		static void ImGuiRender();

	private:
		static std::unordered_map<std::string, std::string> sPublicFieldStringValue;

	private:
		friend PublicField;
	};
}
