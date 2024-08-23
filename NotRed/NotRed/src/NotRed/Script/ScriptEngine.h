#pragma once

#include "NotRed/Core/Core.h"
#include "NotRed/Core/TimeFrame.h"

#include <string>

#include "NotRed/Scene/Components.h"
#include "NotRed/Scene/Entity.h"

extern "C" {
	typedef struct _MonoObject MonoObject;
	typedef struct _MonoClassField MonoClassField;
}

namespace NR
{
	enum class FieldType
	{
		None,
		Int, Float, UnsignedInt,
		String,
		Vec2, Vec3, Vec4
	};

	struct EntityInstance;

	struct PublicField
	{
		std::string Name;
		FieldType Type;

		PublicField(const std::string& name, FieldType type)
			: Name(name), Type(type) {}

		template<typename T>
		T GetValue() const
		{
			T value;
			GetValue_Internal(&value);
			return value;
		}

		template<typename T>
		void SetValue(T value) const
		{
			SetValue_Internal(&value);
		}
	private:
		EntityInstance* mEntityInstance;
		MonoClassField* mMonoClassField;

		void SetValue_Internal(void* value) const;
		void GetValue_Internal(void* outValue) const;

		friend class ScriptEngine;
	};

	using ScriptModuleFieldMap = std::unordered_map<std::string, std::vector<PublicField>>;

	class ScriptEngine
	{
	public:
		static void Init(const std::string& assemblyPath);
		static void Shutdown();

		static void CreateEntity(Entity entity);

		static void InitEntity(ScriptComponent& script, uint32_t entityID, uint32_t sceneID);
		static void UpdateEntity(uint32_t entityID, float dt);

		static const ScriptModuleFieldMap& GetFieldMap();
	};
}
