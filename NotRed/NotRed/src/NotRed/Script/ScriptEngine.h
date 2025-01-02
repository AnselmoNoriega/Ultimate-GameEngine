#pragma once

#include <string>

#include "NotRed/Core/Core.h"

#include "NotRed/Scene/Components.h"
#include "NotRed/Scene/Entity.h"

#include "ScriptModuleField.h"

extern "C" 
{
	typedef struct _MonoObject MonoObject;
	typedef struct _MonoClass MonoClass;
}

namespace NR 
{
	struct EntityScriptClass;

	struct EntityInstance
	{
		EntityScriptClass* ScriptClass = nullptr;

		uint32_t Handle = 0;
		Scene* SceneInstance = nullptr;

		MonoObject* GetInstance();
		bool IsRuntimeAvailable() const;
	};

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
		static void UpdateEntity(Entity entity, float dt);
		static void PhysicsUpdateEntity(Entity entity, float fixedTimeStep);

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

		static bool ModuleExists(const std::string& moduleName);
		static std::string StripNamespace(const std::string& nameSpace, const std::string& moduleName);

		static void InitScriptEntity(Entity entity);
		static void ShutdownScriptEntity(Entity entity, const std::string& moduleName);
		static void InstantiateEntityClass(Entity entity);

		static EntityInstanceMap& GetEntityInstanceMap();
		static EntityInstanceData& GetEntityInstanceData(UUID sceneID, UUID entityID);

		// Debug
		static void ImGuiRender();

	private:
		static std::unordered_map<std::string, std::string> sPublicFieldStringValue;

	private:
		friend PublicField;
	};

}