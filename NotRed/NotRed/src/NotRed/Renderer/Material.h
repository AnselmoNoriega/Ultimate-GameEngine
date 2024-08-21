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
		Blend = (1 << 2)
	};

	class Material
	{
	public:
		static Ref<Material> Create(const Ref<Shader>& shader);

		Material(const Ref<Shader>& shader);
		virtual ~Material();

		void Bind() const;

		uint32_t GetFlags() const { return mMaterialFlags; }
		void SetFlag(MaterialFlag flag) { mMaterialFlags |= (uint32_t)flag; }

		template <typename T>
		void Set(const std::string& name, const T& value)
		{
			auto decl = FindUniformDeclaration(name);

			if (!decl)
			{
				return;
			}

			NR_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
			auto& buffer = GetUniformBufferTarget(decl);
			buffer.Write((byte*)&value, decl->GetSize(), decl->GetOffset());

			for (auto mi : mMaterialInstances)
			{
				mi->MaterialValueUpdated(decl);
			}
		}

		void Set(const std::string& name, const Ref<Texture>& texture)
		{
			auto decl = FindResourceDeclaration(name);
			uint32_t slot = decl->GetRegister();
			if (mTextures.size() <= slot)
			{
				mTextures.resize((size_t)slot + 1);
			}
			mTextures[slot] = texture;
		}

		void Set(const std::string& name, const Ref<Texture2D>& texture)
		{
			Set(name, (const Ref<Texture>&)texture);
		}

		void Set(const std::string& name, const Ref<TextureCube>& texture)
		{
			Set(name, (const Ref<Texture>&)texture);
		}

	private:
		friend class MaterialInstance;

	private:
		void AllocateStorage();
		void ShaderReloaded();
		void BindTextures() const;

		ShaderUniformDeclaration* FindUniformDeclaration(const std::string& name);
		ShaderResourceDeclaration* FindResourceDeclaration(const std::string& name);
		Buffer& GetUniformBufferTarget(ShaderUniformDeclaration* uniformDeclaration);

	private:
		Ref<Shader> mShader;
		std::unordered_set<MaterialInstance*> mMaterialInstances;

		Buffer mVSUniformStorageBuffer;
		Buffer mPSUniformStorageBuffer;
		std::vector<Ref<Texture>> mTextures;

		uint32_t mMaterialFlags;
	};

	class MaterialInstance
	{
	public:
		static Ref<MaterialInstance> Create(const Ref<Material>& material);

		MaterialInstance(const Ref<Material>& material);
		virtual ~MaterialInstance();

		template <typename T>
		void Set(const std::string& name, const T& value)
		{
			auto decl = mMaterial->FindUniformDeclaration(name);
			if (!decl)
			{
				NR_CORE_WARN("Cannot find material property: ", name);
				return;
			}

			auto& buffer = GetUniformBufferTarget(decl);
			buffer.Write((byte*)&value, decl->GetSize(), decl->GetOffset());

			mOverriddenValues.insert(name);
		}

		void Set(const std::string& name, const Ref<Texture>& texture)
		{
			auto decl = mMaterial->FindResourceDeclaration(name);
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

		void Set(const std::string& name, const Ref<Texture2D>& texture)
		{
			Set(name, (const Ref<Texture>&)texture);
		}

		void Set(const std::string& name, const Ref<TextureCube>& texture)
		{
			Set(name, (const Ref<Texture>&)texture);
		}

		void Bind() const;

		uint32_t GetFlags() const { return mMaterial->mMaterialFlags; }
		bool IsFlagActive(MaterialFlag flag) const { return (uint32_t)flag & mMaterial->mMaterialFlags; }
		void ModifyFlags(MaterialFlag flag, bool addFlag = true);

		Ref<Shader >GetShader() { return mMaterial->mShader; }

	private:
		friend class Material;
		
	private:
		void AllocateStorage();
		void ShaderReloaded();
		Buffer& GetUniformBufferTarget(ShaderUniformDeclaration* uniformDeclaration);
		void MaterialValueUpdated(ShaderUniformDeclaration* decl);

	private:
		Ref<Material> mMaterial;

		Buffer mVSUniformStorageBuffer;
		Buffer mPSUniformStorageBuffer;
		std::vector<Ref<Texture>> mTextures;

		// TODO: This is temporary; come up with a proper system to track overrides
		std::unordered_set<std::string> mOverriddenValues;
	};

}