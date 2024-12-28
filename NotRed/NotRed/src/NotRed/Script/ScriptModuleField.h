#pragma once

#include <string>
#include <unordered_map>

extern "C" 
{
	typedef struct _MonoClassField MonoClassField;
}

namespace NR
{
	enum class FieldType
	{
		None, 
		Float, Int, UnsignedInt, 
		String, 
		Vec2, Vec3, Vec4, 
		ClassReference, Asset
	};

	inline const char* FieldTypeToString(FieldType type)
	{
		switch (type)
		{
		case FieldType::Float:       return "Float";
		case FieldType::Int:         return "Int";
		case FieldType::UnsignedInt: return "UnsignedInt";
		case FieldType::String:      return "String";
		case FieldType::Vec2:        return "Vec2";
		case FieldType::Vec3:        return "Vec3";
		case FieldType::Vec4:        return "Vec4";
		}
		return "Unknown";
	}

	struct EntityInstance;
	
	struct PublicField
	{
		std::string Name;
		std::string TypeName;
		FieldType Type;
		PublicField() = default;
		PublicField(const std::string& name, const std::string& typeName, FieldType type);
		PublicField(const PublicField& other);
		PublicField(PublicField&& other);
		~PublicField();

		PublicField& operator=(const PublicField& other);
		
		void CopyStoredValueToRuntime(EntityInstance& entityInstance);
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
		T GetRuntimeValue(EntityInstance& entityInstance) const
		{
			T value;
			GetRuntimeValue_Internal(entityInstance, &value);
			return value;
		}

		template<typename T>
		void SetRuntimeValue(EntityInstance& entityInstance, T value) const
		{
			SetRuntimeValue_Internal(entityInstance, &value);
		}

		void SetStoredValueRaw(void* src);
		void* GetStoredValueRaw() { return mStoredValueBuffer; }
		void SetRuntimeValueRaw(EntityInstance& entityInstance, void* src);
		void* GetRuntimeValueRaw(EntityInstance& entityInstance);

	private:
		uint8_t* AllocateBuffer(FieldType type);
		void SetStoredValue_Internal(void* value) const;
		void GetStoredValue_Internal(void* outValue) const;
		void SetRuntimeValue_Internal(EntityInstance& entityInstance, void* value) const;
		void GetRuntimeValue_Internal(EntityInstance& entityInstance, void* outValue) const;

	private:
		MonoClassField* mMonoClassField;
		uint8_t* mStoredValueBuffer = nullptr;

	private:
		friend class ScriptEngine;
	};

	using ScriptModuleFieldMap = std::unordered_map<std::string, std::unordered_map<std::string, PublicField>>;
}