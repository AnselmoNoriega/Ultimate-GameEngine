#pragma once

#include <string>
#include <unordered_map>

extern "C" 
{
	typedef struct _MonoClassField MonoClassField;
	typedef struct _MonoProperty MonoProperty;
}

namespace NR
{
	enum class FieldType
	{
		None,
		Bool,
		Float, Int, UnsignedInt, 
		String, 
		Vec2, Vec3, Vec4, 
		ClassReference, Asset,
		Entity
	};

	inline const char* FieldTypeToString(FieldType type)
	{
		switch (type)
		{
		case FieldType::Bool:		 return "Bool";
		case FieldType::Float:       return "Float";
		case FieldType::Int:         return "Int";
		case FieldType::UnsignedInt: return "UnsignedInt";
		case FieldType::String:      return "String";
		case FieldType::Vec2:        return "Vec2";
		case FieldType::Vec3:        return "Vec3";
		case FieldType::Vec4:        return "Vec4";
		case FieldType::Asset:		 return "Asset";
		case FieldType::Entity:		 return "Entity";
		}
		return "Unknown";
	}

	struct EntityInstance;

	struct PublicField
	{
		std::string Name;
		std::string TypeName;
		FieldType Type;
		bool IsReadOnly;

		PublicField() = default;
		PublicField(const std::string& name, const std::string& typeName, FieldType type, bool isReadOnly = false);
		PublicField(const PublicField& other);
		PublicField(PublicField&& other);
		~PublicField();

		PublicField& operator=(const PublicField& other);

		void CopyStoredValueToRuntime(EntityInstance& entityInstance);
		void CopyStoredValueFromRuntime(EntityInstance& entityInstance);

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
		T GetRuntimeValue(EntityInstance& entityInstance) const
		{
			T value;
			GetRuntimeValue_Internal(entityInstance, &value);
			return value;
		}

		template<>
		std::string GetRuntimeValue(EntityInstance& entityInstance) const
		{
			std::string value;
			GetRuntimeValue_Internal(entityInstance, value);
			return value;
		}

		template<typename T>
		void SetRuntimeValue(EntityInstance& entityInstance, T value)
		{
			SetRuntimeValue_Internal(entityInstance, &value);
		}

		template<>
		void SetRuntimeValue(EntityInstance& entityInstance, const std::string& value)
		{
			SetRuntimeValue_Internal(entityInstance, value);
		}

		void SetStoredValueRaw(void* src);
		void* GetStoredValueRaw() { return mStoredValueBuffer; }

		void SetRuntimeValueRaw(EntityInstance& entityInstance, void* src);
		void* GetRuntimeValueRaw(EntityInstance& entityInstance);

	private:
		MonoClassField* mMonoClassField = nullptr;
		MonoProperty* mMonoProperty = nullptr;
		uint8_t* mStoredValueBuffer = nullptr;

		uint8_t* AllocateBuffer(FieldType type);
		void SetStoredValue_Internal(void* value);
		void GetStoredValue_Internal(void* outValue) const;
		void SetRuntimeValue_Internal(EntityInstance& entityInstance, void* value);
		void SetRuntimeValue_Internal(EntityInstance& entityInstance, const std::string& value);
		void GetRuntimeValue_Internal(EntityInstance& entityInstance, void* outValue) const;
		void GetRuntimeValue_Internal(EntityInstance& entityInstance, std::string& outValue) const;

	private:
		friend class ScriptEngine;
	};

	using ScriptModuleFieldMap = std::unordered_map<std::string, std::unordered_map<std::string, PublicField>>;

}