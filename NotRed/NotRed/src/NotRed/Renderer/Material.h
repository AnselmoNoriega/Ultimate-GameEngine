#pragma once

#include "NotRed/Core/Core.h"

#include "NotRed/Renderer/Shader.h"
#include "NotRed/Renderer/Texture.h"

#include <unordered_set>

namespace NR 
{

	enum class MaterialFlag
	{
		None = (1 << 0),
		DepthTest = (1 << 1),
		Blend = (1 << 2),
		TwoSided = (1 << 3)
	};

	class Material// : public RefCounted TODO
	{
		friend class Material;
	public:
		static Ref<Material> Create(const Ref<Shader>& shader, const std::string& name = "");
		virtual ~Material() {}

		virtual void Invalidate() = 0;

		virtual void Set(const std::string& name, float value) = 0;
		virtual void Set(const std::string& name, int value) = 0;
		virtual void Set(const std::string& name, uint32_t value) = 0;
		virtual void Set(const std::string& name, bool value) = 0;
		virtual void Set(const std::string& name, const glm::vec2& value) = 0;
		virtual void Set(const std::string& name, const glm::vec3& value) = 0;
		virtual void Set(const std::string& name, const glm::vec4& value) = 0;
		virtual void Set(const std::string& name, const glm::mat3& value) = 0;
		virtual void Set(const std::string& name, const glm::mat4& value) = 0;

		virtual void Set(const std::string& name, const Ref<Texture2D>& texture) = 0;
		virtual void Set(const std::string& name, const Ref<TextureCube>& texture) = 0;
		//virtual void Set(const std::string& name, const Ref<Image2D>& image) = 0;

		virtual float& GetFloat(const std::string& name) = 0;
		virtual int32_t& GetInt(const std::string& name) = 0;
		virtual uint32_t& GetUInt(const std::string& name) = 0;
		virtual bool& GetBool(const std::string& name) = 0;
		virtual glm::vec2& GetVector2(const std::string& name) = 0;
		virtual glm::vec3& GetVector3(const std::string& name) = 0;
		virtual glm::vec4& GetVector4(const std::string& name) = 0;
		virtual glm::mat3& GetMatrix3(const std::string& name) = 0;
		virtual glm::mat4& GetMatrix4(const std::string& name) = 0;

		virtual Ref<Texture2D> GetTexture2D(const std::string& name) = 0;
		virtual Ref<TextureCube> GetTextureCube(const std::string& name) = 0;

		virtual Ref<Texture2D> TryGetTexture2D(const std::string& name) = 0;
		virtual Ref<TextureCube> TryGetTextureCube(const std::string& name) = 0;

		virtual uint32_t GetFlags() const = 0;
		virtual bool GetFlag(MaterialFlag flag) const = 0;
		virtual void SetFlag(MaterialFlag flag, bool value = true) = 0;

		virtual Ref<Shader> GetShader() = 0;
		virtual const std::string& GetName() const = 0;
	};

}