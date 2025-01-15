#include "nrpch.h"
#include "ScriptEngine.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/attrdefs.h>
#include <mono/metadata/mono-gc.h>

#include <iostream>
#include <chrono>
#include <thread>

#include "ScriptEngineRegistry.h"

#include "NotRed/Scene/Scene.h"
#include "NotRed/Debug/Profiler.h"

#include "imgui.h"

namespace NR
{
	static MonoDomain* sCurrentMonoDomain = nullptr;
	static MonoDomain* sNewMonoDomain = nullptr;
	static std::string sCoreAssemblyPath;
	static Ref<Scene> sSceneContext;

	// Assembly images
	MonoImage* sAppAssemblyImage = nullptr;
	MonoImage* sCoreAssemblyImage = nullptr;

	static EntityInstanceMap sEntityInstanceMap;

	static MonoMethod* GetMethod(MonoImage* image, const std::string& methodDesc);

	static MonoMethod* sExceptionMethod = nullptr;
	static MonoClass* sEntityClass = nullptr;

	struct EntityScriptClass
	{
		std::string FullName;
		std::string ClassName;
		std::string NamespaceName;

		MonoClass* Class = nullptr;
		MonoMethod* Constructor = nullptr;
		MonoMethod* CreateMethod = nullptr;
		MonoMethod* DestroyMethod = nullptr;
		MonoMethod* UpdateMethod = nullptr;
		MonoMethod* PhysicsUpdateMethod = nullptr;

		// Physics
		MonoMethod* CollisionBeginMethod = nullptr;
		MonoMethod* CollisionEndMethod = nullptr;
		MonoMethod* TriggerBeginMethod = nullptr;
		MonoMethod* TriggerEndMethod = nullptr;
		MonoMethod* Collision2DBeginMethod = nullptr;
		MonoMethod* Collision2DEndMethod = nullptr;

		void InitClassMethods(MonoImage* image)
		{
			Constructor = GetMethod(sCoreAssemblyImage, "NR.Entity:.ctor(ulong)");
			CreateMethod = GetMethod(image, FullName + ":Init()");
			UpdateMethod = GetMethod(image, FullName + ":Update(single)");
			PhysicsUpdateMethod = GetMethod(image, FullName + ":FixedUpdate(single)");

			// Physics (Entity class)
			CollisionBeginMethod = GetMethod(sCoreAssemblyImage, "NR.Entity:CollisionBegin(ulong)");
			CollisionEndMethod = GetMethod(sCoreAssemblyImage, "NR.Entity:CollisionEnd(ulong)");
			TriggerBeginMethod = GetMethod(sCoreAssemblyImage, "NR.Entity:TriggerBegin(ulong)");
			TriggerEndMethod = GetMethod(sCoreAssemblyImage, "NR.Entity:TriggerEnd(ulong)");
			Collision2DBeginMethod = GetMethod(sCoreAssemblyImage, "NR.Entity:Collision2DBegin(ulong)");
			Collision2DEndMethod = GetMethod(sCoreAssemblyImage, "NR.Entity:Collision2DEnd(ulong)");
		}
	};

	MonoObject* EntityInstance::GetInstance()
	{
		NR_CORE_ASSERT(Handle, "Entity has not been instantiated!");
		return mono_gchandle_get_target(Handle);
	}

	bool EntityInstance::IsRuntimeAvailable() const
	{
		return Handle != 0;
	}

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

	static void InitMono()
	{
		NR_CORE_ASSERT(!sCurrentMonoDomain, "Mono has already been initialised!");
		mono_set_assemblies_path("mono/lib");
		auto domain = mono_jit_init("NotRed");
	}

	static void ShutdownMono()
	{
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
			std::cout << "mono_class_from_name failed" << std::endl;
		}

		return monoClass;
	}

	static uint32_t Instantiate(EntityScriptClass& scriptClass)
	{
		NR_PROFILE_FUNC();

		MonoObject* instance = mono_object_new(sCurrentMonoDomain, scriptClass.Class);
		if (!instance)
		{
			std::cout << "mono_object_new failed" << std::endl;
		}

		mono_runtime_object_init(instance);
		uint32_t handle = mono_gchandle_new(instance, false);
		return handle;
	}

	static void Destroy(uint32_t handle)
	{
		mono_gchandle_free(handle);
	}

	static MonoMethod* GetMethod(MonoImage* image, const std::string& methodDesc)
	{
		NR_CORE_VERIFY(image);
		MonoMethodDesc* desc = mono_method_desc_new(methodDesc.c_str(), NULL);
		if (!desc)
		{
			NR_CORE_ERROR("[ScriptEngine] mono_method_desc_new failed ({0})", methodDesc);
		}

		MonoMethod* method = mono_method_desc_search_in_image(desc, image);
		if (!method)
		{
			NR_CORE_WARN("[ScriptEngine] mono_method_desc_search_in_image failed ({0})", methodDesc);
		}

		return method;
	}

	static std::string GetStringProperty(const char* propertyName, MonoClass* classType, MonoObject* object)
	{
		MonoProperty* property = mono_class_get_property_from_name(classType, propertyName);
		MonoMethod* getterMethod = mono_property_get_get_method(property);
		MonoString* string = (MonoString*)mono_runtime_invoke(getterMethod, object, NULL, NULL);
		return string != nullptr ? std::string(mono_string_to_utf8(string)) : "";
	}

	static MonoObject* CallMethod(MonoObject* object, MonoMethod* method, void** params = nullptr)
	{
		MonoObject* pException = nullptr;
		MonoObject* result = mono_runtime_invoke(method, object, params, &pException);
		if (pException)
		{
			MonoClass* exceptionClass = mono_object_get_class(pException);
			MonoType* exceptionType = mono_class_get_type(exceptionClass);
			const char* typeName = mono_type_get_name(exceptionType);
			std::string message = GetStringProperty("Message", exceptionClass, pException);
			std::string stackTrace = GetStringProperty("StackTrace", exceptionClass, pException);

			NR_CONSOLE_LOG_ERROR("{0}: {1}. Stack Trace: {2}", typeName, message, stackTrace);

			void* args[] = { pException };
			MonoObject* result = mono_runtime_invoke(sExceptionMethod, nullptr, args, nullptr);
		}
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
		return mono_string_new(sCurrentMonoDomain, "Hello!");
	}

	static bool sPostLoadCleanup = false;

	bool ScriptEngine::LoadRuntimeAssembly(const std::string& path)
	{
		sCoreAssemblyPath = path;
		if (sCurrentMonoDomain)
		{
			sNewMonoDomain = mono_domain_create_appdomain((char*)"NotRed Runtime", nullptr);
			mono_domain_set(sNewMonoDomain, false);
			sPostLoadCleanup = true;
		}
		else
		{
			sCurrentMonoDomain = mono_domain_create_appdomain((char*)"NotRed Runtime", nullptr);
			mono_domain_set(sCurrentMonoDomain, false);
			sPostLoadCleanup = false;
		}

		sCoreAssembly = LoadAssembly(sCoreAssemblyPath);
		if (!sCoreAssembly)
		{
			return false;
		}

		sCoreAssemblyImage = GetAssemblyImage(sCoreAssembly);

		sExceptionMethod = GetMethod(sCoreAssemblyImage, "NR.RuntimeException:OnException(object)");
		sEntityClass = mono_class_from_name(sCoreAssemblyImage, "NR", "Entity");

		return true;
	}

	bool ScriptEngine::LoadAppAssembly(const std::string& path)
	{
		if (sAppAssembly)
		{
			sAppAssembly = nullptr;
			sAppAssemblyImage = nullptr;
			return ReloadAssembly(path);
		}

		auto appAssembly = LoadAssembly(path);
		if (!appAssembly)
		{
			return false;
		}

		auto appAssemblyImage = GetAssemblyImage(appAssembly);
		ScriptEngineRegistry::RegisterAll();

		if (sPostLoadCleanup)
		{
			mono_domain_unload(sCurrentMonoDomain);
			sCurrentMonoDomain = sNewMonoDomain;
			sNewMonoDomain = nullptr;
		}

		sAppAssembly = appAssembly;
		sAppAssemblyImage = appAssemblyImage;
		return true;
	}

	bool ScriptEngine::ReloadAssembly(const std::string& path)
	{
		if (!LoadRuntimeAssembly(sCoreAssemblyPath))
		{
			return false;
		}

		if (!LoadAppAssembly(path))
		{
			return false;
		}

		if (sEntityInstanceMap.size())
		{
			Ref<Scene> scene = ScriptEngine::GetCurrentSceneContext();
			NR_CORE_ASSERT(scene, "No active scene!");
			if (const auto& entityInstanceMap = sEntityInstanceMap.find(scene->GetID()); entityInstanceMap != sEntityInstanceMap.end())
			{
				const auto& entityMap = scene->GetEntityMap();
				for (auto& [entityID, entityInstanceData] : entityInstanceMap->second)
				{
					NR_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");
					InitScriptEntity(entityMap.at(entityID));
				}
			}
		}

		return true;
	}

	void ScriptEngine::Init(const std::string& assemblyPath)
	{
		InitMono();
		LoadRuntimeAssembly(assemblyPath);
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

	static std::unordered_map<std::string, MonoClass*> sClasses;
	void ScriptEngine::SetSceneContext(const Ref<Scene>& scene)
	{
		sClasses.clear();
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
			for (auto& [moduleName, srcFieldMap] : srcEntityMap.at(entityID).ModuleFieldMap)
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
		if (entityInstance.ScriptClass->CreateMethod)
			CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->CreateMethod);
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

	void ScriptEngine::PhysicsUpdateEntity(Entity entity, float fixedTimeStep)
	{
		NR_PROFILE_FUNC();
		EntityInstance& entityInstance = GetEntityInstanceData(entity.GetSceneID(), entity.GetID()).Instance;
		if (entityInstance.ScriptClass->PhysicsUpdateMethod)
		{
			void* args[] = { &fixedTimeStep };
			CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->PhysicsUpdateMethod, args);
		}
	}

	void ScriptEngine::Collision2DBegin(Entity entity, Entity other)
	{
		NR_PROFILE_FUNC();
		EntityInstance& entityInstance = GetEntityInstanceData(entity.GetSceneID(), entity.GetID()).Instance;
		if (entityInstance.ScriptClass->Collision2DBeginMethod)
		{
			UUID id = other.GetID();
			void* args[] = { &id };
			CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->Collision2DBeginMethod, args);
		}
	}

	void ScriptEngine::Collision2DEnd(Entity entity, Entity other)
	{
		NR_PROFILE_FUNC();
		EntityInstance& entityInstance = GetEntityInstanceData(entity.GetSceneID(), entity.GetID()).Instance;
		if (entityInstance.ScriptClass->Collision2DEndMethod)
		{
			UUID id = other.GetID();
			void* args[] = { &id };
			CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->Collision2DEndMethod, args);
		}
	}

	void ScriptEngine::CollisionBegin(Entity entity, Entity other)
	{
		NR_PROFILE_FUNC();
		EntityInstance& entityInstance = GetEntityInstanceData(entity.GetSceneID(), entity.GetID()).Instance;
		if (entityInstance.ScriptClass->CollisionBeginMethod)
		{
			UUID id = other.GetID();
			void* args[] = { &id };
			CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->CollisionBeginMethod, args);
		}
	}

	void ScriptEngine::CollisionEnd(Entity entity, Entity other)
	{
		NR_PROFILE_FUNC();
		EntityInstance& entityInstance = GetEntityInstanceData(entity.GetSceneID(), entity.GetID()).Instance;
		if (entityInstance.ScriptClass->CollisionEndMethod)
		{
			UUID id = other.GetID();
			void* args[] = { &id };
			CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->CollisionEndMethod, args);
		}
	}

	void ScriptEngine::TriggerBegin(Entity entity, Entity other)
	{
		NR_PROFILE_FUNC();
		EntityInstance& entityInstance = GetEntityInstanceData(entity.GetSceneID(), entity.GetID()).Instance;
		if (entityInstance.ScriptClass->TriggerBeginMethod)
		{
			UUID id = other.GetID();
			void* args[] = { &id };
			CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->TriggerBeginMethod, args);
		}
	}

	void ScriptEngine::TriggerEnd(Entity entity, Entity other)
	{
		NR_PROFILE_FUNC();
		EntityInstance& entityInstance = GetEntityInstanceData(entity.GetSceneID(), entity.GetID()).Instance;
		if (entityInstance.ScriptClass->TriggerEndMethod)
		{
			UUID id = other.GetID();
			void* args[] = { &id };
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

		MonoClass* clazz = mono_class_from_name(sCoreAssemblyImage, namespaceName.c_str(), className.c_str());
		MonoObject* obj = mono_object_new(mono_domain_get(), clazz);

		if (callConstructor)
		{
			MonoMethodDesc* desc = mono_method_desc_new(parameterList.c_str(), NULL);
			MonoMethod* constructor = mono_method_desc_search_in_class(desc, clazz);
			MonoObject* exception = nullptr;
			mono_runtime_invoke(constructor, obj, parameters, &exception);
		}

		return obj;
	}

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
		if (sEntityInstanceMap.find(sceneID) != sEntityInstanceMap.end())
		{
			auto& entityMap = sEntityInstanceMap.at(sceneID);

			if (entityMap.find(entityID) != entityMap.end())
			{
				entityMap.erase(entityID);
			}
		}
	}

	bool ScriptEngine::ModuleExists(const std::string& moduleName)
	{
		NR_PROFILE_FUNC();

		if (!sAppAssemblyImage)
		{
			return false;
		}

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
		if (!monoClass)
		{
			return false;
		}

		auto isEntitySubclass = mono_class_is_subclass_of(monoClass, sEntityClass, 0);
		return isEntitySubclass;
	}

	std::string ScriptEngine::StripNamespace(const std::string& nameSpace, const std::string& moduleName)
	{
		std::string name = moduleName;
		size_t pos = name.find(nameSpace + ".");
		if (pos == 0)
		{
			{
				name.erase(pos, nameSpace.length() + 1);
			}
		}
		return name;
	}

	static FieldType GetNotRedFieldType(MonoType* monoType)
	{
		int type = mono_type_get_type(monoType);
		switch (type)
		{
		case MONO_TYPE_R4: return FieldType::Float;
		case MONO_TYPE_I4: return FieldType::Int;
		case MONO_TYPE_U4: return FieldType::UnsignedInt;
		case MONO_TYPE_STRING: return FieldType::String;
		case MONO_TYPE_CLASS:
		{
			char* name = mono_type_get_name(monoType);
			if (strcmp(name, "NR.Prefab") == 0) return FieldType::Asset;
			if (strcmp(name, "NR.Entity") == 0) return FieldType::Entity;
			return FieldType::ClassReference;
		}
		case MONO_TYPE_VALUETYPE:
		{
			char* name = mono_type_get_name(monoType);
			if (strcmp(name, "NR.Vector2") == 0) return FieldType::Vec2;
			if (strcmp(name, "NR.Vector3") == 0) return FieldType::Vec3;
			if (strcmp(name, "NR.Vector4") == 0) return FieldType::Vec4;
		}
		}
		return FieldType::None;
	}

	void ScriptEngine::InitScriptEntity(Entity entity)
	{
		NR_PROFILE_FUNC();

		Scene* scene = entity.mScene;
		UUID id = entity.GetComponent<IDComponent>().ID;
		NR_CORE_TRACE("InitScriptEntity {0} ({1})", id, (uint64_t)entity.mEntityHandle);
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

		ScriptComponent& scriptComponent = entity.GetComponent<ScriptComponent>();
		ScriptModuleFieldMap& moduleFieldMap = scriptComponent.ModuleFieldMap;
		auto& fieldMap = moduleFieldMap[moduleName];

		std::unordered_map<std::string, PublicField> oldFields;
		oldFields.reserve(fieldMap.size());
		for (auto& [fieldName, field] : fieldMap)
		{
			oldFields.emplace(fieldName, std::move(field));
		}

		entityInstance.Handle = Instantiate(*entityInstance.ScriptClass);

		void* param[] = { &id };
		CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->Constructor, param);

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
				FieldType notRedFieldType = GetNotRedFieldType(fieldType);

				char* typeName = mono_type_get_name(fieldType);

				auto old = oldFields.find(name);
				if ((old != oldFields.end()) && (old->second.TypeName == typeName))
				{
					fieldMap.emplace(name, std::move(oldFields.at(name)));
					PublicField& field = fieldMap.at(name);
					field.mMonoClassField = iter;
					continue;
				}

				if (notRedFieldType == FieldType::ClassReference)
				{
					continue;
				}

				// TODO: Attributes
				MonoCustomAttrInfo* attr = mono_custom_attrs_from_field(scriptClass.Class, iter);

				PublicField field = { name, typeName, notRedFieldType };
				field.mMonoClassField = iter;
				field.CopyStoredValueToRuntime(entityInstance);
				fieldMap.emplace(name, std::move(field));
			}
		}

		{
			MonoProperty* iter;
			void* ptr = 0;
			while ((iter = mono_class_get_properties(scriptClass.Class, &ptr)) != NULL)
			{
				const char* propertyName = mono_property_get_name(iter);

				if (oldFields.find(propertyName) != oldFields.end())
				{
					fieldMap.emplace(propertyName, std::move(oldFields.at(propertyName)));
					PublicField& field = fieldMap.at(propertyName);
					field.mStoredValueBuffer = (uint8_t*)iter;
					continue;
				}

				MonoMethod* propertySetter = mono_property_get_set_method(iter);
				MonoMethod* propertyGetter = mono_property_get_get_method(iter);

				uint32_t setterFlags = 0;
				uint32_t getterFlags = 0;

				bool isReadOnly = false;
				MonoType* monoType = nullptr;

				if (propertySetter)
				{
					void* i = nullptr;
					MonoMethodSignature* sig = mono_method_signature(propertySetter);
					setterFlags = mono_method_get_flags(propertySetter, nullptr);
					isReadOnly = (setterFlags & MONO_METHOD_ATTR_PRIVATE) != 0;
					monoType = mono_signature_get_params(sig, &i);
				}

				if (propertyGetter)
				{
					MonoMethodSignature* sig = mono_method_signature(propertyGetter);
					getterFlags = mono_method_get_flags(propertyGetter, nullptr);

					if (monoType != nullptr)
					{
						monoType = mono_signature_get_return_type(sig);
					}

					if ((getterFlags & MONO_METHOD_ATTR_PRIVATE) != 0)
					{
						continue;
					}
				}

				if ((setterFlags & MONO_METHOD_ATTR_STATIC) != 0)
					continue;

				FieldType type = GetNotRedFieldType(monoType);
				if (type == FieldType::ClassReference)
				{
					continue;
				}

				char* typeName = mono_type_get_name(monoType);

				PublicField field = { propertyName, typeName, type, isReadOnly };
				field.mMonoProperty = iter;
				field.CopyStoredValueFromRuntime(entityInstance);
				fieldMap.emplace(propertyName, std::move(field));
			}
		}

		Destroy(entityInstance.Handle);
	}

	void ScriptEngine::ShutdownScriptEntity(Entity entity, const std::string& moduleName)
	{
		{
			EntityInstanceData& entityInstanceData = GetEntityInstanceData(entity.GetSceneID(), entity.GetID());
			ScriptModuleFieldMap& moduleFieldMap = entityInstanceData.ModuleFieldMap;
			if (moduleFieldMap.find(moduleName) != moduleFieldMap.end())
			{
				moduleFieldMap.erase(moduleName);
			}
		}

		{
			ScriptComponent& scriptComponent = entity.GetComponent<ScriptComponent>();
			ScriptModuleFieldMap& moduleFieldMap = scriptComponent.ModuleFieldMap;
			if (moduleFieldMap.find(moduleName) != moduleFieldMap.end())
			{
				moduleFieldMap.erase(moduleName);
			}
		}
	}


	void ScriptEngine::InstantiateEntityClass(Entity entity)
	{
		NR_PROFILE_FUNC();

		Scene* scene = entity.mScene;
		UUID id = entity.GetComponent<IDComponent>().ID;
		NR_CORE_TRACE("InstantiateEntityClass {0} ({1})", id, (uint64_t)entity.mEntityHandle);
		ScriptComponent& scriptComponent = entity.GetComponent<ScriptComponent>();
		auto& moduleName = scriptComponent.ModuleName;

		EntityInstanceData& entityInstanceData = GetEntityInstanceData(scene->GetID(), id);
		EntityInstance& entityInstance = entityInstanceData.Instance;
		NR_CORE_ASSERT(entityInstance.ScriptClass);
		entityInstance.Handle = Instantiate(*entityInstance.ScriptClass);

		void* param[] = { &id };
		CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->Constructor, param);

		ScriptModuleFieldMap& moduleFieldMap = scriptComponent.ModuleFieldMap;
		if (moduleFieldMap.find(moduleName) != moduleFieldMap.end())
		{
			auto& publicFields = moduleFieldMap.at(moduleName);
			for (auto& [name, field] : publicFields)
			{
				field.CopyStoredValueToRuntime(entityInstance);
			}
		}
	}

	EntityInstanceData& ScriptEngine::GetEntityInstanceData(UUID sceneID, UUID entityID)
	{
		NR_CORE_ASSERT(sEntityInstanceMap.find(sceneID) != sEntityInstanceMap.end(), "Invalid scene ID!");
		auto& entityIDMap = sEntityInstanceMap.at(sceneID);

		// Entity hasn't been initialized yet
		if (entityIDMap.find(entityID) == entityIDMap.end())
		{
			ScriptEngine::InitScriptEntity(sSceneContext->GetEntityMap().at(entityID));
		}

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
		case FieldType::String:      return 8;
		case FieldType::Vec2:        return 4 * 2;
		case FieldType::Vec3:        return 4 * 3;
		case FieldType::Vec4:        return 4 * 4;
		case FieldType::Asset:       return 8;
		case FieldType::Entity:		 return 8;
		default:
		{
			NR_CORE_ASSERT(false, "Unknown field type!");
			return 0;
		}
		}
	}

	PublicField::PublicField(const std::string& name, const std::string& typeName, FieldType type, bool isReadOnly)
		: Name(name), TypeName(typeName), Type(type), IsReadOnly(isReadOnly)
	{
		if (Type != FieldType::String)
			mStoredValueBuffer = AllocateBuffer(Type);
	}

	PublicField::PublicField(const PublicField& other)
		: Name(other.Name), TypeName(other.TypeName), Type(other.Type), IsReadOnly(other.IsReadOnly)
	{
		if (Type != FieldType::String)
		{
			mStoredValueBuffer = AllocateBuffer(Type);
			memcpy(mStoredValueBuffer, other.mStoredValueBuffer, GetFieldSize(Type));
		}
		else
		{
			mStoredValueBuffer = other.mStoredValueBuffer;
		}

		mMonoClassField = other.mMonoClassField;
		mMonoProperty = other.mMonoProperty;
	}

	PublicField& PublicField::operator=(const PublicField& other)
	{
		if (&other != this)
		{
			Name = other.Name;
			TypeName = other.TypeName;
			Type = other.Type;
			IsReadOnly = other.IsReadOnly;
			
			mMonoClassField = other.mMonoClassField;
			mMonoProperty = other.mMonoProperty;

			if (Type != FieldType::String)
			{
				mStoredValueBuffer = AllocateBuffer(Type);
				memcpy(mStoredValueBuffer, other.mStoredValueBuffer, GetFieldSize(Type));
			}
			else
			{
				mStoredValueBuffer = other.mStoredValueBuffer;
			}
		}

		return *this;
	}

	PublicField::PublicField(PublicField&& other)
	{
		Name = std::move(other.Name);
		TypeName = std::move(other.TypeName);
		Type = other.Type;
		IsReadOnly = other.IsReadOnly;
		mMonoClassField = other.mMonoClassField;
		mMonoProperty = other.mMonoProperty;
		mStoredValueBuffer = other.mStoredValueBuffer;

		other.mMonoClassField = nullptr;
		other.mMonoProperty = nullptr;
		if (Type != FieldType::String)
		{
			other.mStoredValueBuffer = nullptr;
		}
	}

	PublicField::~PublicField()
	{
		if (Type != FieldType::String)
		{
			delete[] mStoredValueBuffer;
		}
	}

	void PublicField::CopyStoredValueToRuntime(EntityInstance& entityInstance)
	{
		NR_CORE_ASSERT(entityInstance.GetInstance());

		if (IsReadOnly)
		{
			return;
		}

		else if (Type == FieldType::Asset || Type == FieldType::Entity)
		{
			// Create Managed Object
			void* params[] = { mStoredValueBuffer };
			MonoObject* obj = ScriptEngine::Construct(TypeName + ":.ctor(ulong)", true, params);
			if (mMonoProperty)
			{
				void* data[] = { obj };
				mono_property_set_value(mMonoProperty, entityInstance.GetInstance(), data, nullptr);
			}
			else
			{
				mono_field_set_value(entityInstance.GetInstance(), mMonoClassField, obj);
			}
		}
		else if (Type == FieldType::String)
		{
			SetRuntimeValue_Internal(entityInstance, *(std::string*)mStoredValueBuffer);
		}
		else
		{
			SetRuntimeValue_Internal(entityInstance, mStoredValueBuffer);
		}
	}

	void PublicField::CopyStoredValueFromRuntime(EntityInstance& entityInstance)
	{
		NR_CORE_ASSERT(entityInstance.GetInstance());

		if (Type == FieldType::String)
		{
			if (mMonoProperty)
			{
				MonoString* str = (MonoString*)mono_property_get_value(mMonoProperty, entityInstance.GetInstance(), nullptr, nullptr);
				ScriptEngine::sPublicFieldStringValue[Name] = mono_string_to_utf8(str);
				mStoredValueBuffer = (uint8_t*)&ScriptEngine::sPublicFieldStringValue[Name];
			}
			else
			{
				MonoString* str;
				mono_field_get_value(entityInstance.GetInstance(), mMonoClassField, &str);
				ScriptEngine::sPublicFieldStringValue[Name] = mono_string_to_utf8(str);
				mStoredValueBuffer = (uint8_t*)&ScriptEngine::sPublicFieldStringValue[Name];
			}
		}
		else 
		{
			if (mMonoProperty)
			{
				MonoObject* result = mono_property_get_value(mMonoProperty, entityInstance.GetInstance(), nullptr, nullptr);
				memcpy(mStoredValueBuffer, mono_object_unbox(result), GetFieldSize(Type));
			}
			else
			{
				mono_field_get_value(entityInstance.GetInstance(), mMonoClassField, mStoredValueBuffer);
			}
		}
	}

	void PublicField::SetStoredValueRaw(void* src)
	{
		if (IsReadOnly)
		{
			return;
		}

		uint32_t size = GetFieldSize(Type);
		memcpy(mStoredValueBuffer, src, size);
	}

	void PublicField::SetRuntimeValueRaw(EntityInstance& entityInstance, void* src)
	{
		NR_CORE_ASSERT(entityInstance.GetInstance());

		if (IsReadOnly)
		{
			return;
		}

		if (mMonoProperty)
		{
			void* data[] = { src };
			mono_property_set_value(mMonoProperty, entityInstance.GetInstance(), data, nullptr);
		}
		else
		{
			mono_field_set_value(entityInstance.GetInstance(), mMonoClassField, src);
		}
	}

	void* PublicField::GetRuntimeValueRaw(EntityInstance& entityInstance)
	{
		NR_CORE_ASSERT(entityInstance.GetInstance());

		uint8_t* outValue = nullptr;
		mono_field_get_value(entityInstance.GetInstance(), mMonoClassField, outValue);
		return outValue;
	}

	uint8_t* PublicField::AllocateBuffer(FieldType type)
	{
		uint32_t size = GetFieldSize(type);
		uint8_t* buffer = new uint8_t[size];
		memset(buffer, 0, size);
		return buffer;
	}

	void PublicField::SetStoredValue_Internal(void* value)
	{
		if (IsReadOnly)
		{
			return;
		}

		uint32_t size = GetFieldSize(Type);
		memcpy(mStoredValueBuffer, value, size);
	}

	void PublicField::GetStoredValue_Internal(void* outValue) const
	{
		uint32_t size = GetFieldSize(Type);
		memcpy(outValue, mStoredValueBuffer, size);
	}

	void PublicField::SetRuntimeValue_Internal(EntityInstance& entityInstance, void* value)
	{
		NR_CORE_ASSERT(entityInstance.GetInstance());

		if (IsReadOnly)
		{
			return;
		}

		if (mMonoProperty)
		{
			void* data[] = { value };
			mono_property_set_value(mMonoProperty, entityInstance.GetInstance(), data, nullptr);
		}
		else
		{
			mono_field_set_value(entityInstance.GetInstance(), mMonoClassField, value);
		}
	}

	void PublicField::SetRuntimeValue_Internal(EntityInstance& entityInstance, const std::string& value)
	{
		if (IsReadOnly)
		{
			return;
		}

		NR_CORE_ASSERT(entityInstance.GetInstance());

		MonoString* monoString = mono_string_new(mono_domain_get(), value.c_str());

		if (mMonoProperty)
		{
			void* data[] = { monoString };
			mono_property_set_value(mMonoProperty, entityInstance.GetInstance(), data, nullptr);
		}
		else
		{
			mono_field_set_value(entityInstance.GetInstance(), mMonoClassField, monoString);
		}
	}

	void PublicField::GetRuntimeValue_Internal(EntityInstance& entityInstance, void* outValue) const
	{
		NR_CORE_ASSERT(entityInstance.GetInstance());

		if (Type == FieldType::Entity)
		{
			MonoObject* obj;
			if (mMonoProperty)
			{
				obj = mono_property_get_value(mMonoProperty, entityInstance.GetInstance(), nullptr, nullptr);
			}
			else
			{
				mono_field_get_value(entityInstance.GetInstance(), mMonoClassField, &obj);
			}

			MonoProperty* idProperty = mono_class_get_property_from_name(entityInstance.ScriptClass->Class, "ID");
			MonoObject* idObject = mono_property_get_value(idProperty, obj, nullptr, nullptr);
			memcpy(outValue, mono_object_unbox(idObject), GetFieldSize(Type));
		}
		else
		{
			if (mMonoProperty)
			{
				MonoObject* result = mono_property_get_value(mMonoProperty, entityInstance.GetInstance(), nullptr, nullptr);
				memcpy(outValue, mono_object_unbox(result), GetFieldSize(Type));
			}
			else
			{
				mono_field_get_value(entityInstance.GetInstance(), mMonoClassField, outValue);
			}
		}
	}

	void PublicField::GetRuntimeValue_Internal(EntityInstance& entityInstance, std::string& outValue) const
	{
		NR_CORE_ASSERT(entityInstance.GetInstance());

		MonoString* monoString;
		if (mMonoProperty)
		{
			monoString = (MonoString*)mono_property_get_value(mMonoProperty, entityInstance.GetInstance(), nullptr, nullptr);
		}
		else
		{
			mono_field_get_value(entityInstance.GetInstance(), mMonoClassField, &monoString);
		}

		outValue = mono_string_to_utf8(monoString);
	}

	// Debug
	void ScriptEngine::ImGuiRender()
	{
		ImGui::Begin("Script Engine Debug");

		float gcHeapSize = (float)mono_gc_get_heap_size();
		float gcUsageSize = (float)mono_gc_get_used_size();
		ImGui::Text("GC Heap Info(Used/Available): %.2fKB / %.2fKB", gcUsageSize / 1024.0f, gcHeapSize / 1024.0f);

		for (auto& [sceneID, entityMap] : sEntityInstanceMap)
		{
			bool opened = ImGui::TreeNode((void*)(uint64_t)sceneID, "Scene (%llx)", sceneID);
			if (opened)
			{
				Ref<Scene> scene = Scene::GetScene(sceneID);
				for (auto& [entityID, entityInstanceData] : entityMap)
				{
					Entity entity = scene->GetEntityMap().at(entityID);
					std::string entityName = "Unnamed Entity";
					if (entity.HasComponent<TagComponent>())
					{
						entityName = entity.GetComponent<TagComponent>().Tag;
					}

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

	std::unordered_map<std::string, std::string> ScriptEngine::sPublicFieldStringValue;
}