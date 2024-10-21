#include "nrpch.h"
#include "GLMaterial.h"

#include "NotRed/Renderer/Renderer.h"

#include "GLShader.h"
#include "GLTexture.h"
#include "GLImage.h"

namespace NR
{
	GLMaterial::GLMaterial(const Ref<Shader>& shader, const std::string& name)
		: mShader(shader), mName(name)
	{
		mShader->AddShaderReloadedCallback(std::bind(&GLMaterial::ShaderReloaded, this));
		AllocateStorage();

		mMaterialFlags |= (uint32_t)MaterialFlag::DepthTest;
		mMaterialFlags |= (uint32_t)MaterialFlag::Blend;
	}

	void GLMaterial::AllocateStorage()
	{
		const auto& shaderBuffers = mShader->GetShaderBuffers();

		if (shaderBuffers.size() > 0)
		{
			uint32_t size = 0;
			for (auto [name, shaderBuffer] : shaderBuffers)
			{
				size += shaderBuffer.Size;
			}

			mUniformStorageBuffer.Allocate(size);
			mUniformStorageBuffer.ZeroInitialize();
		}
	}

	void GLMaterial::Invalidate()
	{

	}

	void GLMaterial::ShaderReloaded()
	{
		//AllocateStorage();
	}

	const ShaderUniform* GLMaterial::FindUniformDeclaration(const std::string& name)
	{
		const auto& shaderBuffers = mShader->GetShaderBuffers();

		NR_CORE_ASSERT(shaderBuffers.size() <= 1 || true, "Currently only support ONE material buffer!");

		for (auto& [buffName, buffer] : shaderBuffers)
		{
			if (buffer.Uniforms.find(name) != buffer.Uniforms.end())
			{
				return &buffer.Uniforms.at(name);
			}
		}
		return nullptr;
	}

	const ShaderResourceDeclaration* GLMaterial::FindResourceDeclaration(const std::string& name)
	{
		auto& resources = mShader->GetResources();
		for (const auto& [n, resource] : resources)
		{
			if (resource.GetName() == name)
			{
				return &resource;
			}
		}
		return nullptr;
	}

	void GLMaterial::Set(const std::string& name, const glm::ivec2& value)
	{
		Set<glm::ivec2>(name, value);
	}

	void GLMaterial::Set(const std::string& name, const glm::ivec3& value)
	{
		Set<glm::ivec3>(name, value);
	}

	void GLMaterial::Set(const std::string& name, const glm::ivec4& value)
	{
		Set<glm::ivec4>(name, value);
	}

	void GLMaterial::Set(const std::string& name, float value)
	{
		Set<float>(name, value);
	}

	void GLMaterial::Set(const std::string& name, int value)
	{
		Set<int>(name, value);
	}

	void GLMaterial::Set(const std::string& name, uint32_t value)
	{
		Set<uint32_t>(name, value);
	}

	void GLMaterial::Set(const std::string& name, bool value)
	{
		Set<uint32_t>(name, (int)value);
	}

	void GLMaterial::Set(const std::string& name, const glm::vec2& value)
	{
		Set<glm::vec2>(name, value);
	}

	void GLMaterial::Set(const std::string& name, const glm::vec3& value)
	{
		Set<glm::vec3>(name, value);
	}

	void GLMaterial::Set(const std::string& name, const glm::vec4& value)
	{
		Set<glm::vec4>(name, value);
	}

	void GLMaterial::Set(const std::string& name, const glm::mat3& value)
	{
		Set<glm::mat3>(name, value);
	}

	void GLMaterial::Set(const std::string& name, const glm::mat4& value)
	{
		Set<glm::mat4>(name, value);
	}

	void GLMaterial::Set(const std::string& name, const Ref<Texture2D>& texture)
	{
		auto decl = FindResourceDeclaration(name);
		if (!decl)
		{
			NR_CORE_WARN("Cannot find material property: ", name);
			return;
		}
		uint32_t slot = decl->GetRegister();
		mTexture2Ds[slot] = texture;
	}

	void GLMaterial::Set(const std::string& name, const Ref<TextureCube>& texture)
	{
		auto decl = FindResourceDeclaration(name);
		if (!decl)
		{
			NR_CORE_WARN("Cannot find material property: ", name);
			return;
		}
		uint32_t slot = decl->GetRegister();
		if (mTextures.size() <= slot)
			mTextures.resize((size_t)slot + 1);
		mTextures[slot] = texture;
	}

	void GLMaterial::Set(const std::string& name, const Ref<Image2D>& image)
	{
		auto decl = FindResourceDeclaration(name);
		if (!decl)
		{
			NR_CORE_WARN("Cannot find material property: ", name);
			return;
		}
		uint32_t slot = decl->GetRegister();
		mImages[slot] = image;
	}

	float& GLMaterial::GetFloat(const std::string& name)
	{
		return Get<float>(name);
	}

	int32_t& GLMaterial::GetInt(const std::string& name)
	{
		return Get<int32_t>(name);
	}

	uint32_t& GLMaterial::GetUInt(const std::string& name)
	{
		return Get<uint32_t>(name);
	}

	bool& GLMaterial::GetBool(const std::string& name)
	{
		return Get<bool>(name);
	}

	glm::vec2& GLMaterial::GetVector2(const std::string& name)
	{
		return Get<glm::vec2>(name);
	}

	glm::vec3& GLMaterial::GetVector3(const std::string& name)
	{
		return Get<glm::vec3>(name);
	}

	glm::vec4& GLMaterial::GetVector4(const std::string& name)
	{
		return Get<glm::vec4>(name);
	}

	glm::mat3& GLMaterial::GetMatrix3(const std::string& name)
	{
		return Get<glm::mat3>(name);
	}

	glm::mat4& GLMaterial::GetMatrix4(const std::string& name)
	{
		return Get<glm::mat4>(name);
	}

	Ref<Texture2D> GLMaterial::GetTexture2D(const std::string& name)
	{
		auto decl = FindResourceDeclaration(name);
		NR_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
		uint32_t slot = decl->GetRegister();
		NR_CORE_ASSERT(slot < mTexture2Ds.size(), "Texture slot is invalid");
		return mTexture2Ds[slot];
	}

	Ref<TextureCube> GLMaterial::TryGetTextureCube(const std::string& name)
	{
		return TryGetResource<TextureCube>(name);
	}

	Ref<Texture2D> GLMaterial::TryGetTexture2D(const std::string& name)
	{
		auto decl = FindResourceDeclaration(name);
		if (!decl)
		{
			return nullptr;
		}

		uint32_t slot = decl->GetRegister();
		if (mTexture2Ds.find(slot) == mTexture2Ds.end())
		{
			return nullptr;
		}

		return mTexture2Ds[slot];
	}

	Ref<TextureCube> GLMaterial::GetTextureCube(const std::string& name)
	{
		return GetResource<TextureCube>(name);
	}

	void GLMaterial::UpdateForRendering()
	{
		Ref<GLShader> shader = mShader.As<GLShader>();

		Renderer::Submit([shader]()
			{
				glUseProgram(shader->GetRendererID());
			});

		const auto& shaderBuffers = GetShader()->GetShaderBuffers();
		NR_CORE_ASSERT(shaderBuffers.size() <= 1, "We currently only support ONE material buffer!");

		if (shaderBuffers.size() > 0)
		{
			const ShaderBuffer& buffer = (*shaderBuffers.begin()).second;

			for (auto& [name, uniform] : buffer.Uniforms)
			{
				switch (uniform.GetType())
				{
				case ShaderUniformType::Bool:
				case ShaderUniformType::UInt:
				{
					uint32_t value = mUniformStorageBuffer.Read<uint32_t>(uniform.GetOffset());
					shader->SetUniform(name, value);
					break;
				}
				case ShaderUniformType::Int:
				{
					int value = mUniformStorageBuffer.Read<int>(uniform.GetOffset());
					shader->SetUniform(name, value);
					break;
				}
				case ShaderUniformType::Float:
				{
					float value = mUniformStorageBuffer.Read<float>(uniform.GetOffset());
					shader->SetUniform(name, value);
					break;
				}
				case ShaderUniformType::Vec2:
				{
					const glm::vec2& value = mUniformStorageBuffer.Read<glm::vec2>(uniform.GetOffset());
					shader->SetUniform(name, value);
					break;
				}
				case ShaderUniformType::Vec3:
				{
					const glm::vec3& value = mUniformStorageBuffer.Read<glm::vec3>(uniform.GetOffset());
					shader->SetUniform(name, value);
					break;
				}
				case ShaderUniformType::Vec4:
				{
					const glm::vec4& value = mUniformStorageBuffer.Read<glm::vec4>(uniform.GetOffset());
					shader->SetUniform(name, value);
					break;
				}
				case ShaderUniformType::Mat3:
				{
					const glm::mat3& value = mUniformStorageBuffer.Read<glm::mat3>(uniform.GetOffset());
					shader->SetUniform(name, value);
					break;
				}
				case ShaderUniformType::Mat4:
				{
					const glm::mat4& value = mUniformStorageBuffer.Read<glm::mat4>(uniform.GetOffset());
					shader->SetUniform(name, value);
					break;
				}
				default:
				{
					NR_CORE_ASSERT(false);
					break;
				}
				}
			}
		}

		for (size_t i = 0; i < mTextures.size(); ++i)
		{
			auto& texture = mTextures[i];
			if (texture)
			{
				Renderer::Submit([i, texture]()
					{
						NR_CORE_ASSERT(texture->GetType() == TextureType::TextureCube);
						Ref<GLTextureCube> glTexture = texture.As<GLTextureCube>();
						glBindTextureUnit(i, glTexture->GetRendererID());
					});
			}
		}

		for (auto [slot, texture] : mTexture2Ds)
		{
			if (texture)
			{
				uint32_t textureSlot = slot;
				Ref<Image2D> image = texture->GetImage();
				Ref<GLImage2D> glImage = image.As<GLImage2D>();
				Renderer::Submit([textureSlot, glImage]()
					{
						glBindSampler(textureSlot, glImage->GetSamplerRendererID());
						glBindTextureUnit(textureSlot, glImage->GetRendererID());
					});
			}
		}

		for (auto [slot, image] : mImages)
		{
			if (image)
			{
				uint32_t textureSlot = slot;
				Ref<GLImage2D> glImage = image.As<GLImage2D>();
				Renderer::Submit([textureSlot, glImage]()
					{
						glBindSampler(textureSlot, glImage->GetSamplerRendererID());
						glBindTextureUnit(textureSlot, glImage->GetRendererID());
					});
			}
		}
	}
}