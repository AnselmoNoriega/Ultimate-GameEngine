#include "nrpch.h"
#include "GLShader.h"

#include <string>
#include <sstream>
#include <limits>

#include <shaderc/shaderc.hpp>
#include <filesystem>

#include <glm/gtc/type_ptr.hpp>

#include "NotRed/Renderer/Renderer.h"

#include "NotRed/Util/FileSystem.h"

namespace NR
{
#define UNIFORmLOGGING 0
#if UNIFORmLOGGING
#define NR_LOG_UNIFORM(...) NR_CORE_WARN(__VA_ARGS__)
#else
#define NR_LOG_UNIFORM
#endif

#define PRINT_SHADERS 1

	namespace Utils
	{
		static const char* GetCacheDirectory()
		{
			return "Assets/Cache/Shader/OpenGL";
		}

		static void CreateCacheDirectoryIfNeeded()
		{
			std::string cacheDirectory = GetCacheDirectory();
			if (!std::filesystem::exists(cacheDirectory))
			{
				std::filesystem::create_directories(cacheDirectory);
			}
		}
	}

	GLShader::GLShader(const std::string& filepath, bool forceRecompile)
		: mAssetPath(filepath)
	{
		size_t fileName = filepath.find_last_of("/\\");
		mName = fileName != std::string::npos ? filepath.substr(fileName + 1) : filepath;

		Reload(forceRecompile);
	}

	Ref<GLShader> GLShader::CreateFromString(const std::string& vertSrc, const std::string& fragSrc, const std::string& computeSrc)
	{
		Ref<GLShader> shader = Ref<GLShader>::Create();
		shader->Load(vertSrc, fragSrc, computeSrc, true);
		return shader;
	}

	void GLShader::Reload(bool forceCompile)
	{
		std::string compute = "";
		ParseFile(mAssetPath + "/" + mName + ".comp", compute, true);

		std::string vert = "";
		std::string frag = "";

		if (compute == "")
		{
			ParseFile(mAssetPath + "/" + mName + ".vert", vert);
			ParseFile(mAssetPath + "/" + mName + ".frag", frag);
		}

		Load(vert, frag, compute, forceCompile);
	}

	void GLShader::Load(const std::string& vertSrc, const std::string& fragSrc, const std::string& computeSrc, bool forceCompile)
	{
		if (computeSrc.empty())
		{
			mShaderSource.insert({ GL_VERTEX_SHADER, vertSrc });
			mShaderSource.insert({ GL_FRAGMENT_SHADER, fragSrc });
		}
		else
		{
			mIsCompute = true;
			mShaderSource.insert({ GL_COMPUTE_SHADER, computeSrc });
		}

		Utils::CreateCacheDirectoryIfNeeded();
		Ref<GLShader> instance = this;
		Renderer::Submit([instance, forceCompile]() mutable
			{
				std::unordered_map<uint32_t, std::vector<uint32_t>> shaderData;
				instance->CompileOrGetVulkanBinary(shaderData, forceCompile);
				instance->CompileOrGetOpenGLBinary(shaderData, forceCompile);
			});
	}

	void GLShader::ParseFile(const std::string& filepath, std::string& output, bool isCompute) const
	{
		std::ifstream in(filepath, std::ios::in | std::ios::binary);

		if (in)
		{
			in.seekg(0, std::ios::end);
			output.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&output[0], output.size());
		}
		else if (!isCompute)
		{
			NR_CORE_ASSERT(false, "Could not open file");
		}

		in.close();
	}

	void GLShader::ClearUniformBuffers()
	{
		sUniformBuffers.clear();
	}

	static const char* GLShaderStageCachedVulkanFileExtension(uint32_t stage)
	{
		switch (stage)
		{
		case GL_VERTEX_SHADER:    return ".cached_vulkan.vert";
		case GL_FRAGMENT_SHADER:  return ".cached_vulkan.frag";
		case GL_COMPUTE_SHADER:   return ".cached_vulkan.comp";
		default:
			NR_CORE_ASSERT(false);
			return "";
		}
	}

	static const char* GLShaderStageCachedOpenGLFileExtension(uint32_t stage)
	{
		switch (stage)
		{
		case GL_VERTEX_SHADER:    return ".cached_opengl.vert";
		case GL_FRAGMENT_SHADER:  return ".cached_opengl.frag";
		case GL_COMPUTE_SHADER:   return ".cached_opengl.comp";
		default:
			NR_CORE_ASSERT(false);
			return "";
		}
	}

	static shaderc_shader_kind GLShaderStageToShaderC(uint32_t stage)
	{
		switch (stage)
		{
		case GL_VERTEX_SHADER:    return shaderc_glsl_vertex_shader;
		case GL_FRAGMENT_SHADER:  return shaderc_glsl_fragment_shader;
		case GL_COMPUTE_SHADER:   return shaderc_glsl_compute_shader;
		default:
			NR_CORE_ASSERT(false);
			return (shaderc_shader_kind)0;
		}
	}

	std::string GetGLShaderFileExtension(uint32_t stage, std::string& assetPath)
	{
		switch (stage)
		{
		case GL_VERTEX_SHADER:    return std::string(assetPath) + ".vert";
		case GL_FRAGMENT_SHADER:  return std::string(assetPath) + ".frag";
		case GL_COMPUTE_SHADER:   return std::string(assetPath) + ".comp";
		default:
			NR_CORE_ASSERT(false);
			return "";
		}
	}

	static const char* GLShaderTypeToString(uint32_t stage)
	{
		switch (stage)
		{
		case GL_VERTEX_SHADER:    return "Vertex";
		case GL_FRAGMENT_SHADER:  return "Fragment";
		case GL_COMPUTE_SHADER:   return "Compute";
		}
		NR_CORE_ASSERT(false);
		return "";
	}

	void GLShader::CompileOrGetVulkanBinary(std::unordered_map<uint32_t, std::vector<uint32_t>>& outputBinary, bool forceCompile)
	{
		std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();
		for (auto [stage, source] : mShaderSource)
		{
			auto extension = GLShaderStageCachedVulkanFileExtension(stage);
			std::string fullShaderPath = mAssetPath + "/" + mName;
			std::filesystem::path shaderPath = GetGLShaderFileExtension(stage, fullShaderPath);
			if (!forceCompile)
			{
				auto path = cacheDirectory / (shaderPath.filename().string() + extension);
				std::string cachedFilePath = path.string();

				FILE* f;
				fopen_s(&f, cachedFilePath.c_str(), "rb");
				if (f)
				{
					fseek(f, 0, SEEK_END);
					uint64_t size = ftell(f);
					fseek(f, 0, SEEK_SET);
					outputBinary[stage] = std::vector<uint32_t>(size / sizeof(uint32_t));
					fread(outputBinary[stage].data(), sizeof(uint32_t), outputBinary[stage].size(), f);
					fclose(f);
				}
			}

			if (outputBinary[stage].size() == 0)
			{
				shaderc::Compiler compiler;
				shaderc::CompileOptions options;
				options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
				options.AddMacroDefinition("OPENGL");
				const bool optimize = false;
				if (optimize)
				{
					options.SetOptimizationLevel(shaderc_optimization_level_performance);
				}

				// Compile shader
				{
					auto& shaderSource = mShaderSource.at(stage);
					shaderSource.erase(0, shaderSource.find_first_not_of("\xEF\xBB\xBF"));
					shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
						shaderSource,
						GLShaderStageToShaderC(stage),
						shaderPath.string().c_str(),
						options);

					if (module.GetCompilationStatus() != shaderc_compilation_status_success)
					{
						NR_CORE_ERROR(module.GetErrorMessage());
						NR_CORE_ASSERT(false);
					}

					const uint8_t* begin = (const uint8_t*)module.cbegin();
					const uint8_t* end = (const uint8_t*)module.cend();
					const ptrdiff_t size = end - begin;

					outputBinary[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());
				}

				// Cache compiled shader
				{
					auto path = cacheDirectory / (shaderPath.filename().string() + extension);
					std::string cachedFilePath = path.string();

					FILE* f;
					fopen_s(&f, cachedFilePath.c_str(), "wb");
					fwrite(outputBinary[stage].data(), sizeof(uint32_t), outputBinary[stage].size(), f);
					fclose(f);
				}
			}
		}
	}

	void GLShader::CompileOrGetOpenGLBinary(const std::unordered_map<uint32_t, std::vector<uint32_t>>& vulkanBinaries, bool forceCompile)
	{
		if (mID)
		{
			glDeleteProgram(mID);
		}

		GLuint program = glCreateProgram();
		mID = program;

		std::vector<GLuint> shaderRendererIDs;
		shaderRendererIDs.reserve(vulkanBinaries.size());

		std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();

		mConstantBufferOffset = 0;
		std::vector<std::vector<uint32_t>> shaderData;
		for (auto [stage, binary] : vulkanBinaries)
		{
			shaderc::Compiler compiler;
			shaderc::CompileOptions options;
			options.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);

			{
				spirv_cross::CompilerGLSL glsl(binary);

				spirv_cross::CompilerGLSL::Options glslOptions = glsl.get_common_options();
				glslOptions.version = 450;
				glslOptions.es = false;
				glslOptions.vulkan_semantics = false;
				glslOptions.emit_push_constant_as_uniform_buffer = true;
				glsl.set_common_options(glslOptions);

				ParseConstantBuffers(glsl);

				std::string fullShaderPath = mAssetPath + "/" + mName;
				std::filesystem::path shaderPath = GetGLShaderFileExtension(stage, fullShaderPath);
				auto path = cacheDirectory / (shaderPath.filename().string() + GLShaderStageCachedOpenGLFileExtension(stage));
				std::string cachedFilePath = path.string();

				std::vector<uint32_t>& shaderStageData = shaderData.emplace_back();

				if (!forceCompile)
				{
					FILE* f;
					fopen_s(&f, cachedFilePath.c_str(), "rb");
					if (f)
					{
						fseek(f, 0, SEEK_END);
						uint64_t size = ftell(f);
						fseek(f, 0, SEEK_SET);
						shaderStageData = std::vector<uint32_t>(size / sizeof(uint32_t));
						fread(shaderStageData.data(), sizeof(uint32_t), shaderStageData.size(), f);
						fclose(f);
					}
				}

				if (!shaderStageData.size())
				{
					std::string source = glsl.compile();
#if PRINT_SHADERS
					printf("=========================================\n");
					printf("%s Shader:\n%s\n", GLShaderTypeToString(stage), source.c_str());
					printf("=========================================\n");
#endif
					shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
						source,
						GLShaderStageToShaderC(stage),
						shaderPath.string().c_str(),
						options);

					if (module.GetCompilationStatus() != shaderc_compilation_status_success)
					{
						NR_CORE_ERROR(module.GetErrorMessage());
						NR_CORE_ASSERT(false);
					}

					shaderStageData = std::vector<uint32_t>(module.cbegin(), module.cend());

					{
						auto path = cacheDirectory / (shaderPath.filename().string() + GLShaderStageCachedOpenGLFileExtension(stage));
						std::string cachedFilePath = path.string();
						FILE* f;
						fopen_s(&f, cachedFilePath.c_str(), "wb");
						fwrite(shaderStageData.data(), sizeof(uint32_t), shaderStageData.size(), f);
						fclose(f);
					}
				}

				GLuint shaderID = glCreateShader(stage);
				glShaderBinary(1, &shaderID, GL_SHADER_BINARY_FORMAT_SPIR_V, shaderStageData.data(), uint32_t(shaderStageData.size() * sizeof(uint32_t)));
				glSpecializeShader(shaderID, "main", 0, nullptr, nullptr);
				glAttachShader(program, shaderID);

				shaderRendererIDs.emplace_back(shaderID);
			}
		}

		glLinkProgram(program);

		// Note the different functions here: glGetProgram* instead of glGetShader*.
		GLint isLinked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
		if (isLinked == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);
			NR_CORE_ERROR("Shader compilation failed ({0}):\n{1}", mAssetPath, &infoLog[0]);

			glDeleteProgram(program);

			for (auto id : shaderRendererIDs)
			{
				glDeleteShader(id);
			}
		}

		// Detach shaders after a successful link. Maybe OpenGL does it already 
		for (auto id : shaderRendererIDs)
		{
			glDetachShader(program, id);
		}

		// Get uniform locations
		for (auto& [bufferName, buffer] : mBuffers)
		{
			for (auto& [name, uniform] : buffer.Uniforms)
			{
				GLint location = glGetUniformLocation(mID, name.c_str());
				if (location == -1)
				{
					NR_CORE_WARN("{0}: could not find uniform location {0}", name);
				}

				mUniformLocations[name] = location;
			}
		}

		for (auto& shaderStageData : shaderData)
		{
			Reflect(shaderStageData);
		}
	}

	static ShaderUniformType SPIRTypeToShaderUniformType(spirv_cross::SPIRType type)
	{
		switch (type.basetype)
		{
		case spirv_cross::SPIRType::Boolean:    return ShaderUniformType::Bool;
		case spirv_cross::SPIRType::Int:
			if (type.vecsize == 1)            return ShaderUniformType::Int;
			if (type.vecsize == 2)            return ShaderUniformType::IVec2;
			if (type.vecsize == 3)            return ShaderUniformType::IVec3;
			if (type.vecsize == 4)            return ShaderUniformType::IVec4;
		case spirv_cross::SPIRType::UInt:       return ShaderUniformType::UInt;
		case spirv_cross::SPIRType::Float:
			if (type.columns == 3)              return ShaderUniformType::Mat3;
			if (type.columns == 4)              return ShaderUniformType::Mat4;

			if (type.vecsize == 1)              return ShaderUniformType::Float;
			if (type.vecsize == 2)              return ShaderUniformType::Vec2;
			if (type.vecsize == 3)              return ShaderUniformType::Vec3;
			if (type.vecsize == 4)              return ShaderUniformType::Vec4;
			else
			{
				NR_CORE_ASSERT(false, "Unknown type!");
				return ShaderUniformType::None;
			}
		case spirv_cross::SPIRType::Struct:     return ShaderUniformType::Struct;
		default:
			NR_CORE_ASSERT(false, "Unknown type!");
			return ShaderUniformType::None;
		}
	}

	void GLShader::Compile(const std::vector<uint32_t>& vertexBinary, const std::vector<uint32_t>& fragmentBinary)
	{

	}

	void GLShader::ParseConstantBuffers(const spirv_cross::CompilerGLSL& compiler)
	{
		spirv_cross::ShaderResources res = compiler.get_shader_resources();
		for (const spirv_cross::Resource& resource : res.uniform_buffers)
		{
			const auto& bufferName = compiler.get_name(resource.id);
			auto& bufferType = compiler.get_type(resource.base_type_id);
			const auto bufferSize = (uint32_t)compiler.get_declared_struct_size(bufferType);

			// Skip empty push constant buffers - these are for the renderer only
			if (bufferName.empty())
			{
				mConstantBufferOffset += bufferSize;
				continue;
			}

			auto location = compiler.get_decoration(resource.id, spv::DecorationLocation);
			const int memberCount = int(bufferType.member_types.size());
			auto& [bufName, bufSize, uniforms] = mBuffers[bufferName];
			bufName = bufferName;
			bufSize = bufferSize - mConstantBufferOffset;
			for (int i = 0; i < memberCount; ++i)
			{
				const auto& type = compiler.get_type(bufferType.member_types[i]);
				const auto& memberName = compiler.get_member_name(bufferType.self, i);
				const auto size = compiler.get_declared_struct_member_size(bufferType, i);
				const auto offset = compiler.type_struct_member_offset(bufferType, i) - mConstantBufferOffset;

				std::string uniformName = bufferName + "." + memberName;
				uniforms[uniformName] = ShaderUniform(uniformName, SPIRTypeToShaderUniformType(type), size, offset);
			}

			mConstantBufferOffset += bufferSize;
		}
	}

	void GLShader::Reflect(std::vector<uint32_t>& data)
	{
		spirv_cross::Compiler comp(data);
		spirv_cross::ShaderResources res = comp.get_shader_resources();

		NR_CORE_TRACE("GLShader::Reflect - {0}", mAssetPath);
		NR_CORE_TRACE("   {0} Uniform Buffers", res.uniform_buffers.size());
		NR_CORE_TRACE("   {0} Storage Buffers", res.storage_buffers.size());
		NR_CORE_TRACE("   {0} Resources", res.sampled_images.size());

		glUseProgram(mID);

		//Uniform buffers
		{
			uint32_t bufferIndex = 0;
			for (const spirv_cross::Resource& resource : res.uniform_buffers)
			{
				auto& bufferType = comp.get_type(resource.base_type_id);
				int memberCount = bufferType.member_types.size();
				uint32_t bindingPoint = comp.get_decoration(resource.id, spv::DecorationBinding);
				uint32_t bufferSize = comp.get_declared_struct_size(bufferType);

				if (sUniformBuffers.find(bindingPoint) == sUniformBuffers.end())
				{
					ShaderUniformBuffer& buffer = sUniformBuffers[bindingPoint];
					buffer.Name = resource.name;
					buffer.BindingPoint = bindingPoint;
					buffer.Size = bufferSize;

					glCreateBuffers(1, &buffer.RendererID);
					glBindBuffer(GL_UNIFORM_BUFFER, buffer.RendererID);
					glBufferData(GL_UNIFORM_BUFFER, buffer.Size, nullptr, GL_DYNAMIC_DRAW);
					glBindBufferBase(GL_UNIFORM_BUFFER, buffer.BindingPoint, buffer.RendererID);
					NR_CORE_TRACE("Created Uniform Buffer at binding point {0} with name '{1}', size is {2} bytes", buffer.BindingPoint, buffer.Name, buffer.Size);

					glBindBuffer(GL_UNIFORM_BUFFER, 0);
				}
				else
				{
					// Validation
					ShaderUniformBuffer& buffer = sUniformBuffers.at(bindingPoint);
					NR_CORE_ASSERT(buffer.Name == resource.name); // Must be the same buffer
					if (bufferSize > buffer.Size) // Resize buffer if needed
					{
						buffer.Size = bufferSize;
						glDeleteBuffers(1, &buffer.RendererID);
						glCreateBuffers(1, &buffer.RendererID);
						glBindBuffer(GL_UNIFORM_BUFFER, buffer.RendererID);
						glBufferData(GL_UNIFORM_BUFFER, buffer.Size, nullptr, GL_DYNAMIC_DRAW);
						glBindBufferBase(GL_UNIFORM_BUFFER, buffer.BindingPoint, buffer.RendererID);

						NR_CORE_TRACE("Resized Uniform Buffer at binding point {0} with name '{1}', size is {2} bytes", buffer.BindingPoint, buffer.Name, buffer.Size);
					}
				}
			}
		}
		//Storage Buffers
		{
			uint32_t bufferIndex = 0;
			for (const spirv_cross::Resource& resource : res.storage_buffers)
			{
				const auto& bufferType = comp.get_type(resource.base_type_id);
				//int memberCount = bufferType.member_types.size();
				const uint32_t bindingPoint = comp.get_decoration(resource.id, spv::DecorationBinding);
				const uint32_t bufferSize = comp.get_declared_struct_size(bufferType);
				if (sUniformBuffers.find(bindingPoint) == sUniformBuffers.end())
				{
					ShaderStorageBuffer& buffer = sStorageBuffers[bindingPoint];
					buffer.Name = resource.name;
					buffer.BindingPoint = bindingPoint;
					buffer.Size = bufferSize;
					glCreateBuffers(1, &buffer.RendererID);
					glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer.RendererID);
					glBufferData(GL_SHADER_STORAGE_BUFFER, buffer.Size, nullptr, GL_DYNAMIC_DRAW);
					glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buffer.BindingPoint, buffer.RendererID);
					NR_CORE_TRACE("Created Storage Buffer at binding point {0} with name '{1}', size is {2} bytes", buffer.BindingPoint, buffer.Name, buffer.Size);
					glBindBuffer(GL_UNIFORM_BUFFER, 0);
				}
				else
				{
					// Validation
					ShaderStorageBuffer& buffer = sStorageBuffers.at(bindingPoint);
					NR_CORE_ASSERT(buffer.Name == resource.name); // Must be the same buffer
					if (bufferSize > buffer.Size) // Resize buffer if needed
					{
						buffer.Size = bufferSize;
						glDeleteBuffers(1, &buffer.RendererID);
						glCreateBuffers(1, &buffer.RendererID);
						glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer.RendererID);
						glBufferData(GL_SHADER_STORAGE_BUFFER, buffer.Size, nullptr, GL_DYNAMIC_DRAW);
						glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buffer.BindingPoint, buffer.RendererID);

						NR_CORE_TRACE("Resized Storage Buffer at binding point {0} with name '{1}', size is {2} bytes", buffer.BindingPoint, buffer.Name, buffer.Size);
					}
				}
			}
		}

		int32_t sampler = 0;
		for (const spirv_cross::Resource& resource : res.sampled_images)
		{
			auto& type = comp.get_type(resource.base_type_id);
			auto binding = comp.get_decoration(resource.id, spv::DecorationBinding);
			const auto& name = resource.name;
			uint32_t dimension = type.image.dim;

			GLint location = glGetUniformLocation(mID, name.c_str());
			mResources[name] = ShaderResourceDeclaration(name, binding, 1);
			glUniform1i(location, binding);
		}
	}

	void GLShader::AddShaderReloadedCallback(const ShaderReloadedCallback& callback)
	{
		mShaderReloadedCallbacks.push_back(callback);
	}

	void GLShader::Bind()
	{
		Renderer::Submit([=]() {
			glUseProgram(mID);
			});
	}

#pragma region Parsing helper functions

	const char* FindToken(const char* str, const std::string& token)
	{
		const char* t = str;
		while (t = strstr(t, token.c_str()))
		{
			bool left = str == t || isspace(t[-1]);
			bool right = !t[token.size()] || isspace(t[token.size()]);
			if (left && right)
			{
				return t;
			}

			t += token.size();
		}
		return nullptr;
	}

	const char* FindToken(const std::string& string, const std::string& token)
	{
		return FindToken(string.c_str(), token);
	}

	std::string GetBlock(const char* str, const char** outPosition)
	{
		const char* end = strstr(str, "}");
		if (!end)
		{
			return str;
		}

		if (outPosition)
		{
			*outPosition = end;
		}
		uint32_t length = end - str + 1;
		return std::string(str, length);
	}

	std::string GetStatement(const char* str, const char** outPosition)
	{
		const char* end = strstr(str, ";");
		if (!end)
		{
			return str;
		}

		if (outPosition)
		{
			*outPosition = end;
		}
		uint32_t length = end - str + 1;
		return std::string(str, length);
	}

	bool StartsWith(const std::string& string, const std::string& start)
	{
		return string.find(start) == 0;
	}

	static bool IsTypeStringResource(const std::string& type)
	{
		if (type == "sampler1D")		  return true;
		if (type == "sampler2D")		  return true;
		if (type == "sampler2DMS")		  return true;
		if (type == "samplerCube")		  return true;
		if (type == "sampler2DShadow")	  return true;
		return false;
	}
#pragma endregion

	int32_t GLShader::GetUniformLocation(const std::string& name) const
	{
		int32_t result = glGetUniformLocation(mID, name.c_str());
		if (result == -1)
		{
			NR_CORE_WARN("Could not find uniform '{0}' in shader", name);
		}

		return result;
	}

	size_t GLShader::GetHash() const
	{
		return std::hash<std::string>{}(mAssetPath);
	}

	const ShaderUniformBuffer& GLShader::FindUniformBuffer(const std::string& name)
	{
		ShaderUniformBuffer* uniformBuffer = nullptr;
		for (auto& [bindingPoint, ub] : sUniformBuffers)
		{
			if (ub.Name == name)
			{
				uniformBuffer = &ub;
				break;
			}
		}
		NR_CORE_ASSERT(uniformBuffer);
		return *uniformBuffer;
	}

	void GLShader::SetUniformBuffer(const ShaderUniformBuffer& buffer, const void* data, uint32_t size, const uint32_t offset)
	{
		glNamedBufferSubData(buffer.RendererID, offset, size, data);
	}

	void GLShader::SetStorageBuffer(const ShaderStorageBuffer& buffer, const void* data, uint32_t size, const uint32_t offset)
	{
		glNamedBufferSubData(buffer.RendererID, offset, size, data);
	}

	void GLShader::SetStorageBuffer(const std::string& name, const void* data, uint32_t size)
	{
		ShaderStorageBuffer* storageBuffer = nullptr;
		for (auto& [bindingPoint, ub] : sStorageBuffers)
		{
			if (ub.Name == name)
			{
				storageBuffer = &ub;
				break;
			}
		}
		NR_CORE_ASSERT(storageBuffer);
		NR_CORE_ASSERT(storageBuffer->Size >= size);
		glNamedBufferSubData(storageBuffer->RendererID, 0, size, data);
	}

	void GLShader::ResizeStorageBuffer(const uint32_t bindingPoint, const uint32_t newSize)
	{
		// Validation
		ShaderStorageBuffer& buffer = sStorageBuffers.at(bindingPoint);
		if (newSize == buffer.Size)
		{
			return;
		}

		buffer.Size = newSize;
		glNamedBufferData(buffer.RendererID, buffer.Size, nullptr, GL_DYNAMIC_DRAW);
		NR_CORE_TRACE("Resized Storage Buffer at binding point {0} with name '{1}', size is {2} bytes", buffer.BindingPoint, buffer.Name, buffer.Size);
	}

	void GLShader::SetUniformBuffer(const std::string& name, const void* data, uint32_t size)
	{
		ShaderUniformBuffer* uniformBuffer = nullptr;
		for (auto& [bindingPoint, ub] : sUniformBuffers)
		{
			if (ub.Name == name)
			{
				uniformBuffer = &ub;
				break;
			}
		}

		NR_CORE_ASSERT(uniformBuffer);
		NR_CORE_ASSERT(uniformBuffer->Size >= size);
		glNamedBufferSubData(uniformBuffer->RendererID, 0, size, data);
	}

	void GLShader::SetUniform(const std::string& fullname, float value)
	{
		Renderer::Submit([=]()
			{
				NR_CORE_ASSERT(mUniformLocations.find(fullname) != mUniformLocations.end());
				GLint location = mUniformLocations.at(fullname);
				glUniform1f(location, value);
			});
	}

	void GLShader::SetUniform(const std::string& fullname, uint32_t value)
	{
		Renderer::Submit([=]()
			{
				NR_CORE_ASSERT(mUniformLocations.find(fullname) != mUniformLocations.end());
				GLint location = mUniformLocations.at(fullname);
				glUniform1ui(location, value);
			});
	}

	void GLShader::SetUniform(const std::string& fullname, int value)
	{
		Renderer::Submit([=]()
			{
				NR_CORE_ASSERT(mUniformLocations.find(fullname) != mUniformLocations.end());
				GLint location = mUniformLocations.at(fullname);
				glUniform1i(location, value);
			});
	}

	void GLShader::SetUniform(const std::string& fullname, const glm::vec2& value)
	{
		Renderer::Submit([=]()
			{
				NR_CORE_ASSERT(mUniformLocations.find(fullname) != mUniformLocations.end());
				GLint location = mUniformLocations.at(fullname);
				glUniform2fv(location, 1, glm::value_ptr(value));
			});
	}

	void GLShader::SetUniform(const std::string& fullname, const glm::ivec2& value)
	{
		Renderer::Submit([=]()
			{
				NR_CORE_ASSERT(mUniformLocations.find(fullname) != mUniformLocations.end());
				GLint location = mUniformLocations.at(fullname);
				glUniform2i(location, value.x, value.y);
			});
	}

	void GLShader::SetUniform(const std::string& fullname, const glm::ivec3& value)
	{
		Renderer::Submit([=]()
			{
				NR_CORE_ASSERT(mUniformLocations.find(fullname) != mUniformLocations.end());
				GLint location = mUniformLocations.at(fullname);
				glUniform3i(location, value.x, value.y, value.z);
			});
	}

	void GLShader::SetUniform(const std::string& fullname, const glm::ivec4& value)
	{
		Renderer::Submit([=]()
			{
				NR_CORE_ASSERT(mUniformLocations.find(fullname) != mUniformLocations.end());
				GLint location = mUniformLocations.at(fullname);
				glUniform4i(location, value.x, value.y, value.z, value.w);
			});
	}

	void GLShader::SetUniform(const std::string& fullname, const glm::vec3& value)
	{
		Renderer::Submit([=]()
			{
				NR_CORE_ASSERT(mUniformLocations.find(fullname) != mUniformLocations.end());
				GLint location = mUniformLocations.at(fullname);
				glUniform3fv(location, 1, glm::value_ptr(value));
			});
	}

	void GLShader::SetUniform(const std::string& fullname, const glm::vec4& value)
	{
		Renderer::Submit([=]()
			{
				NR_CORE_ASSERT(mUniformLocations.find(fullname) != mUniformLocations.end());
				GLint location = mUniformLocations.at(fullname);
				glUniform4fv(location, 1, glm::value_ptr(value));
			});
	}

	void GLShader::SetUniform(const std::string& fullname, const glm::mat3& value)
	{
		Renderer::Submit([=]()
			{
				NR_CORE_ASSERT(mUniformLocations.find(fullname) != mUniformLocations.end());
				GLint location = mUniformLocations.at(fullname);
				glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
			});
	}

	void GLShader::SetUniform(const std::string& fullname, const glm::mat4& value)
	{
		Renderer::Submit([=]()
			{
				NR_CORE_ASSERT(mUniformLocations.find(fullname) != mUniformLocations.end());
				GLint location = mUniformLocations.at(fullname);
				glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
			});
	}

	void GLShader::SetIntArray(const std::string& name, int* values, uint32_t size)
	{
		Renderer::Submit([=]() {
			UploadUniformIntArray(name, values, size);
			});
	}

	const ShaderResourceDeclaration* GLShader::GetShaderResource(const std::string& name)
	{
		if (mResources.find(name) == mResources.end())
			return nullptr;

		return &mResources.at(name);
	}

	void GLShader::UploadUniformInt(uint32_t location, int32_t value)
	{
		glUniform1i(location, value);
	}


	void GLShader::UploadUniformIntArray(uint32_t location, int32_t* values, int32_t count)
	{
		glUniform1iv(location, count, values);
	}

	void GLShader::UploadUniformFloat(uint32_t location, float value)
	{
		glUniform1f(location, value);
	}

	void GLShader::UploadUniformFloat2(uint32_t location, const glm::vec2& value)
	{
		glUniform2f(location, value.x, value.y);
	}

	void GLShader::UploadUniformFloat3(uint32_t location, const glm::vec3& value)
	{
		glUniform3f(location, value.x, value.y, value.z);
	}

	void GLShader::UploadUniformFloat4(uint32_t location, const glm::vec4& value)
	{
		glUniform4f(location, value.x, value.y, value.z, value.w);
	}

	void GLShader::UploadUniformMat3(uint32_t location, const glm::mat3& value)
	{
		glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
	}

	void GLShader::UploadUniformMat4(uint32_t location, const glm::mat4& value)
	{
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
	}

	void GLShader::UploadUniformMat4Array(uint32_t location, const glm::mat4& values, uint32_t count)
	{
		glUniformMatrix4fv(location, count, GL_FALSE, glm::value_ptr(values));
	}

	void GLShader::UploadUniformInt(const std::string& name, int32_t value)
	{
		int32_t location = GetUniformLocation(name);
		glUniform1i(location, value);
	}

	void GLShader::UploadUniformUInt(const std::string& name, uint32_t value)
	{
		int32_t location = GetUniformLocation(name);
		glUniform1ui(location, value);
	}

	void GLShader::UploadUniformIntArray(const std::string& name, int32_t* values, uint32_t count)
	{
		int32_t location = GetUniformLocation(name);
		glUniform1iv(location, count, values);
	}

	void GLShader::UploadUniformFloat(const std::string& name, float value)
	{
		glUseProgram(mID);
		auto location = glGetUniformLocation(mID, name.c_str());
		if (location != -1)
		{
			glUniform1f(location, value);
		}
		else
		{
			NR_LOG_UNIFORM("Uniform '{0}' not found!", name);
		}
	}

	void GLShader::UploadUniformFloat2(const std::string& name, const glm::vec2& values)
	{
		glUseProgram(mID);
		auto location = glGetUniformLocation(mID, name.c_str());
		if (location != -1)
		{
			glUniform2f(location, values.x, values.y);
		}
		else
		{
			NR_LOG_UNIFORM("Uniform '{0}' not found!", name);
		}
	}


	void GLShader::UploadUniformFloat3(const std::string& name, const glm::vec3& values)
	{
		glUseProgram(mID);
		auto location = glGetUniformLocation(mID, name.c_str());
		if (location != -1)
		{
			glUniform3f(location, values.x, values.y, values.z);
		}
		else
		{
			NR_LOG_UNIFORM("Uniform '{0}' not found!", name);
		}
	}

	void GLShader::UploadUniformFloat4(const std::string& name, const glm::vec4& values)
	{
		glUseProgram(mID);
		auto location = glGetUniformLocation(mID, name.c_str());
		if (location != -1)
			glUniform4f(location, values.x, values.y, values.z, values.w);
		else
		{
			NR_LOG_UNIFORM("Uniform '{0}' not found!", name);
		}
	}

	void GLShader::UploadUniformMat4(const std::string& name, const glm::mat4& values)
	{
		glUseProgram(mID);
		auto location = glGetUniformLocation(mID, name.c_str());
		if (location != -1)
		{
			glUniformMatrix4fv(location, 1, GL_FALSE, (const float*)&values);
		}
		else
		{
			NR_LOG_UNIFORM("Uniform '{0}' not found!", name);
		}
	}

}