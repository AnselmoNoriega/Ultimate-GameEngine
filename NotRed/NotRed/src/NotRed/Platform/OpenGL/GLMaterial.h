#pragma once

#include "NotRed/Renderer/Material.h"

#include <map>

namespace NR
{
	class GLMaterial : public Material
	{
	public:
		GLMaterial(const Ref<Shader>& shader, const std::string& name = "");
		~GLMaterial() override = default;

		void Invalidate() override;

		void Set(const std::string& name, uint32_t value) override;

		void Set(const std::string& name, bool value) override;
		void Set(const std::string& name, int value) override;
		void Set(const std::string& name, float value) override;

		void Set(const std::string& name, const glm::vec2& value) override;
		void Set(const std::string& name, const glm::vec3& value) override;
		void Set(const std::string& name, const glm::vec4& value) override;

		void Set(const std::string& name, const glm::mat3& value) override;
		void Set(const std::string& name, const glm::mat4& value) override;

		void Set(const std::string& name, const Ref<Texture2D>& texture) override;
		void Set(const std::string& name, const Ref<TextureCube>& texture) override;
		void Set(const std::string& name, const Ref<Image2D>& image) override;

		uint32_t& GetUInt(const std::string& name) override;

		bool& GetBool(const std::string& name) override;
		int32_t& GetInt(const std::string& name) override;
		float& GetFloat(const std::string& name) override;

		glm::vec2& GetVector2(const std::string& name) override;
		glm::vec3& GetVector3(const std::string& name) override;
		glm::vec4& GetVector4(const std::string& name) override;

		glm::mat3& GetMatrix3(const std::string& name) override;
		glm::mat4& GetMatrix4(const std::string& name) override;

		Ref<Texture2D> GetTexture2D(const std::string& name) override;
		Ref<TextureCube> GetTextureCube(const std::string& name) override;

		Ref<Texture2D> TryGetTexture2D(const std::string& name) override;
		Ref<TextureCube> TryGetTextureCube(const std::string& name) override;

		template <typename T>
		void Set(const std::string& name, const T& value)
		{
			auto decl = FindUniformDeclaration(name);
			NR_CORE_ASSERT(decl, "Could not find uniform!");
			if (!decl)
			{
				return;
			}

			auto& buffer = mUniformStorageBuffer;
			buffer.Write((byte*)&value, decl->GetSize(), decl->GetOffset());
		}

		void Set(const std::string& name, const Ref<Texture>& texture)
		{
			auto decl = FindResourceDeclaration(name);
			if (!decl)
			{
				NR_CORE_WARN("Cannot find material property: ", name);
				return;
			}
			uint32_t slot = decl->GetRegister();
			if (mTextures.size() <= slot)
			{
				mTextures.resize((size_t)slot + 1);
			}
			mTextures[slot] = texture;
		}

		template<typename T>
		T& Get(const std::string& name)
		{
			auto decl = FindUniformDeclaration(name);
			NR_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
			auto& buffer = mUniformStorageBuffer;
			return buffer.Read<T>(decl->GetOffset());
		}

		template<typename T>
		Ref<T> GetResource(const std::string& name)
		{
			auto decl = FindResourceDeclaration(name);
			NR_CORE_ASSERT(decl, "Could not find Resource with name 'x'");
			uint32_t slot = decl->GetRegister();
			NR_CORE_ASSERT(slot < mTextures.size(), "Texture slot is invalid!");
			return Ref<T>(mTextures[slot]);
		}

		template<typename T>
		Ref<T> TryGetResource(const std::string& name)
		{
			auto decl = FindResourceDeclaration(name);
			if (!decl)
			{
				return nullptr;
			}

			uint32_t slot = decl->GetRegister();
			if (slot >= mTextures.size())
			{
				return nullptr;
			}

			return Ref<T>(mTextures[slot]);
		}

		uint32_t GetFlags() const override { return mMaterialFlags; }
		bool GetFlag(MaterialFlag flag) const override { return (uint32_t)flag & mMaterialFlags; }
		void ModifyFlags(MaterialFlag flag, bool isAdding = true) override
		{
			if (isAdding)
			{
				mMaterialFlags |= (uint32_t)flag;
			}
			else
			{
				mMaterialFlags &= ~(uint32_t)flag;
			}
		}

		Ref<Shader> GetShader() override { return mShader; }
		const std::string& GetName() const override { return mName; }

		Buffer GetUniformStorageBuffer() { return mUniformStorageBuffer; }

		void UpdateForRendering();

	private:
		void AllocateStorage();
		void ShaderReloaded();

		const ShaderUniform* FindUniformDeclaration(const std::string& name);
		const ShaderResourceDeclaration* FindResourceDeclaration(const std::string& name);

	private:
		Ref<Shader> mShader;
		std::string mName;

		uint32_t mMaterialFlags = 0;

		Buffer mUniformStorageBuffer;
		std::vector<Ref<Texture>> mTextures;
		std::map<uint32_t, Ref<Image2D>> mImages;
		std::map<uint32_t, Ref<Texture2D>> mTexture2Ds;
	};

}