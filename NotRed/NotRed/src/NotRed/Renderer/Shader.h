#pragma once

#include "NotRed/Core/Core.h"
#include "NotRed/Renderer/RendererTypes.h"
#include "NotRed/Core/Buffer.h"
#include "NotRed/Renderer/ShaderUniform.h"

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace NR
{
	enum class ShaderUniformType
	{
		None,
		Bool, Int, IVec2, IVec3, IVec4, UInt, Float, 
		Vec2, Vec3, Vec4, 
		Mat3, Mat4,
		Struct
	};

	class ShaderUniform
	{
	public:
		ShaderUniform() = default;
		ShaderUniform(std::string name, ShaderUniformType type, uint32_t size, uint32_t offset);

		static const std::string& UniformTypeToString(ShaderUniformType type);

		const std::string& GetName() const { return mName; }
		ShaderUniformType GetType() const { return mType; }
		uint32_t GetSize() const { return mSize; }
		uint32_t GetOffset() const { return mOffset; }

	private:
		std::string mName;
		ShaderUniformType mType = ShaderUniformType::None;
		uint32_t mSize = 0;
		uint32_t mOffset = 0;
	};

	struct ShaderUniformBuffer
	{
		std::string Name;
		uint32_t Index;
		uint32_t BindingPoint;
		uint32_t Size;
		uint32_t RendererID;
		std::vector<ShaderUniform> Uniforms;
	};

	struct ShaderStorageBuffer
	{
		std::string Name;
		uint32_t Index;
		uint32_t BindingPoint;
		uint32_t Size;
		uint32_t RendererID;
	};

	struct ShaderBuffer
	{
		std::string Name;
		uint32_t Size = 0;
		std::unordered_map<std::string, ShaderUniform> Uniforms;
	};

	class Shader : public RefCounted
	{
	public:
		virtual ~Shader() = default;

		using ShaderReloadedCallback = std::function<void()>;

		static Ref<Shader> Create(const std::string& filepath, bool forceCompile = false);
		Ref<Shader> CreateFromString(const std::string& vertSrc, const std::string& fragSrc, const std::string& computeSrc = "");

		virtual void Reload(bool forceCompile = false) = 0;

		virtual size_t GetHash() const = 0;

		virtual const std::unordered_map<std::string, ShaderBuffer>& GetShaderBuffers() const = 0;
		virtual const std::unordered_map<std::string, ShaderResourceDeclaration>& GetResources() const = 0;

		virtual void AddShaderReloadedCallback(const ShaderReloadedCallback& callback) = 0;

		virtual const std::string& GetName() const = 0;

		static std::vector<Ref<Shader>> sAllShaders;
	};

	class ShaderLibrary : public RefCounted
	{
	public:
		ShaderLibrary();
		~ShaderLibrary();

		void Add(const Ref<Shader>& shader);
		void Load(const std::string& path, bool forceCompile = false);
		void Load(const std::string& name, const std::string& path);

		const Ref<Shader>& Get(const std::string& name) const;

	private:
		std::unordered_map<std::string, Ref<Shader>> mShaders;
	};
}