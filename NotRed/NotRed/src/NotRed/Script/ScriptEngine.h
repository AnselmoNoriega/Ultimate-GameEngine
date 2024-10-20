#pragma once

#include "NotRed/Core/Core.h"
#include "NotRed/Core/TimeFrame.h"

#include <string>

#include "NotRed/Scene/Components.h"
#include "NotRed/Scene/Entity.h"

extern "C" {
	typedef struct _MonoObject MonoObject;
	typedef struct _MonoClassField MonoClassField;
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
		ClassReference
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

		PublicField(const std::string& name, const std::string& typeName, FieldType type);
		PublicField(const PublicField&) = delete;
		PublicField(PublicField&& other);
		~PublicField();

		void CopyStoredValueToRuntime();
		bool IsRuntimeAvailable() const;

		template<typename T>
		T GetStoredValue() const
		{
			T value;
			GetStoredValue_Internal(&value);
			return value;
		}

		template<typename T>
		void SetStoredValue(T value) const
		{
			SetStoredValue_Internal(&value);
		}

		template<typename T>
		T GetValue() const
		{
			T value;
			GetRuntimeValue_Internal(&value);
			return value;
		}

		template<typename T>
		void SetValue(T value) const
		{
			SetRuntimeValue_Internal(&value);
		}

		void SetStoredValueRaw(void* src);
		void* GetStoredValueRaw() { return mStoredValueBuffer; }

		void SetRuntimeValueRaw(void* src);
		void* GetRuntimeValueRaw();

	private:
		uint8_t* AllocateBuffer(FieldType type);
		void SetStoredValue_Internal(void* value) const;
		void GetStoredValue_Internal(void* outValue) const;
		void SetRuntimeValue_Internal(void* value) const;
		void GetRuntimeValue_Internal(void* outValue) const;

	private:
		EntityInstance* mEntityInstance;
		MonoClassField* mMonoClassField;
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

		static void Collision2DBegin(Entity entity);
		static void Collision2DEnd(Entity entity);

		static void CollisionBegin(Entity entity);
		static void CollisionEnd(Entity entity);

		static void TriggerBegin(Entity entity);
		static void TriggerEnd(Entity entity);

		static MonoObject* Construct(const std::string& fullName, bool callConstructor = true, void** parameters = nullptr);
		static MonoClass* GetCoreClass(const std::string& fullName);

		static bool IsEntityModuleValid(Entity entity);

		static void ScriptComponentDestroyed(UUID sceneID, UUID entityID);

		static bool ModuleExists(const std::string& moduleName);
		static void InitScriptEntity(Entity entity);
		static void ShutdownScriptEntity(Entity entity, const std::string& moduleName);
		static void InstantiateEntityClass(Entity entity);

		static EntityInstanceMap& GetEntityInstanceMap();
		static EntityInstanceData& GetEntityInstanceData(UUID sceneID, UUID entityID);

		static void ImGuiRender();
	};
}
