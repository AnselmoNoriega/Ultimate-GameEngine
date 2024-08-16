#pragma once

#include "NotRed/Core/Core.h"

#include "NotRed/Renderer/Shader.h"
#include "NotRed/Renderer/Texture.h"

#include <unordered_set>

namespace NR
{
	class Material
	{
		friend class MaterialInstance;
	public:
		Material(const Ref<Shader>& shader);
		virtual ~Material();

		void Bind() const;

		template <typename T>
		void Set(const std::string& name, const T& value)
		{
			auto decl = FindUniformDeclaration(name);
			// NR_CORE_ASSERT(decl, "Could not find uniform with name '{0}'", name);
			NR_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
			auto& buffer = GetUniformBufferTarget(decl);
			buffer.Write((byte*)&value, decl->GetSize(), decl->GetOffset());

			for (auto mi : mMaterialInstances)
				mi->OnMaterialValueUpdated(decl);
		}

		void Set(const std::string& name, const Ref<Texture>& texture)
		{
			auto decl = FindResourceDeclaration(name);
			uint32_t slot = decl->GetRegister();
			if (mTextures.size() <= slot)
				mTextures.resize((size_t)slot + 1);
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
		void AllocateStorage();
		void OnShaderReloaded();
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

		int32_t mRenderFlags = 0;
	};

	class MaterialInstance
	{
		friend class Material;
	public:
		MaterialInstance(const Ref<Material>& material);
		virtual ~MaterialInstance();

		template <typename T>
		void Set(const std::string& name, const T& value)
		{
			auto decl = mMaterial->FindUniformDeclaration(name);
			// NR_CORE_ASSERT(decl, "Could not find uniform with name '{0}'", name);
			NR_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
			auto& buffer = GetUniformBufferTarget(decl);
			buffer.Write((byte*)&value, decl->GetSize(), decl->GetOffset());

			mOverriddenValues.insert(name);
		}

		void Set(const std::string& name, const Ref<Texture>& texture)
		{
			auto decl = mMaterial->FindResourceDeclaration(name);
			uint32_t slot = decl->GetRegister();
			if (mTextures.size() <= slot)
				mTextures.resize((size_t)slot + 1);
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
	private:
		void AllocateStorage();
		void OnShaderReloaded();
		Buffer& GetUniformBufferTarget(ShaderUniformDeclaration* uniformDeclaration);
		void OnMaterialValueUpdated(ShaderUniformDeclaration* decl);
	private:
		Ref<Material> mMaterial;

		Buffer mVSUniformStorageBuffer;
		Buffer mPSUniformStorageBuffer;
		std::vector<Ref<Texture>> mTextures;

		// TODO: This is temporary; come up with a proper system to track overrides
		std::unordered_set<std::string> mOverriddenValues;
	};

}