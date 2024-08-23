#include "nrpch.h"
#include "Material.h"

namespace NR
{
	Ref<Material> Material::Create(const Ref<Shader>& shader)
	{
		return Ref<Material>::Create(shader);
	}

	Material::Material(const Ref<Shader>& shader)
		: mShader(shader)
	{
		mShader->AddShaderReloadedCallback(std::bind(&Material::ShaderReloaded, this));
		AllocateStorage();

		mMaterialFlags |= (uint32_t)MaterialFlag::DepthTest;
		mMaterialFlags |= (uint32_t)MaterialFlag::Blend;
	}

	Material::~Material()
	{
	}

	void Material::AllocateStorage()
	{
		if (mShader->HasVSMaterialUniformBuffer())
		{
			const auto& vsBuffer = mShader->GetVSMaterialUniformBuffer();
			mVSUniformStorageBuffer.Allocate(vsBuffer.GetSize());
			mVSUniformStorageBuffer.ZeroInitialize();
		}

		if (mShader->HasPSMaterialUniformBuffer())
		{
			const auto& psBuffer = mShader->GetPSMaterialUniformBuffer();
			mPSUniformStorageBuffer.Allocate(psBuffer.GetSize());
			mPSUniformStorageBuffer.ZeroInitialize();
		}
	}

	void Material::ShaderReloaded()
	{
		AllocateStorage();

		for (auto mi : mMaterialInstances)
		{
			mi->ShaderReloaded();
		}
	}

	ShaderUniformDeclaration* Material::FindUniformDeclaration(const std::string& name)
	{
		if (mVSUniformStorageBuffer)
		{
			auto& declarations = mShader->GetVSMaterialUniformBuffer().GetUniformDeclarations();
			for (ShaderUniformDeclaration* uniform : declarations)
			{
				if (uniform->GetName() == name)
				{
					return uniform;
				}
			}
		}

		if (mPSUniformStorageBuffer)
		{
			auto& declarations = mShader->GetPSMaterialUniformBuffer().GetUniformDeclarations();
			for (ShaderUniformDeclaration* uniform : declarations)
			{
				if (uniform->GetName() == name)
				{
					return uniform;
				}
			}
		}
		return nullptr;
	}

	ShaderResourceDeclaration* Material::FindResourceDeclaration(const std::string& name)
	{
		auto& resources = mShader->GetResources();
		for (ShaderResourceDeclaration* resource : resources)
		{
			if (resource->GetName() == name)
			{
				return resource;
			}
		}
		return nullptr;
	}

	Buffer& Material::GetUniformBufferTarget(ShaderUniformDeclaration* uniformDeclaration)
	{
		switch (uniformDeclaration->GetDomain())
		{
		case ShaderDomain::Vertex:    return mVSUniformStorageBuffer;
		case ShaderDomain::Pixel:     return mPSUniformStorageBuffer;
		}

		NR_CORE_ASSERT(false, "Invalid uniform declaration domain! Material does not support this shader type.");
		return mVSUniformStorageBuffer;
	}

	void Material::Bind()
	{
		mShader->Bind();

		if (mVSUniformStorageBuffer)
		{
			mShader->SetVSMaterialUniformBuffer(mVSUniformStorageBuffer);
		}

		if (mPSUniformStorageBuffer)
		{
			mShader->SetPSMaterialUniformBuffer(mPSUniformStorageBuffer);
		}

		BindTextures();
	}

	void Material::BindTextures()
	{
		for (size_t i = 0; i < mTextures.size(); ++i)
		{
			auto& texture = mTextures[i];
			if (texture)
			{
				texture->Bind(i);
			}
		}
	}

	//Instance-------------------------------------------

	Ref<MaterialInstance> MaterialInstance::Create(const Ref<Material>& material)
	{
		return Ref<MaterialInstance>::Create(material);
	}

	MaterialInstance::MaterialInstance(const Ref<Material>& material)
		: mMaterial(material)
	{
		mMaterial->mMaterialInstances.insert(this);
		AllocateStorage();
	}

	MaterialInstance::~MaterialInstance()
	{
		mMaterial->mMaterialInstances.erase(this);
	}

	void MaterialInstance::ShaderReloaded()
	{
		AllocateStorage();
		mOverriddenValues.clear();
	}

	void MaterialInstance::AllocateStorage()
	{
		if (mMaterial->mShader->HasVSMaterialUniformBuffer())
		{
			const auto& vsBuffer = mMaterial->mShader->GetVSMaterialUniformBuffer();
			mVSUniformStorageBuffer.Allocate(vsBuffer.GetSize());
			memcpy(mVSUniformStorageBuffer.Data, mMaterial->mVSUniformStorageBuffer.Data, vsBuffer.GetSize());
		}

		if (mMaterial->mShader->HasPSMaterialUniformBuffer())
		{
			const auto& psBuffer = mMaterial->mShader->GetPSMaterialUniformBuffer();
			mPSUniformStorageBuffer.Allocate(psBuffer.GetSize());
			memcpy(mPSUniformStorageBuffer.Data, mMaterial->mPSUniformStorageBuffer.Data, psBuffer.GetSize());
		}
	}

	void MaterialInstance::ModifyFlags(MaterialFlag flag, bool addFlag)
	{
		if (addFlag)
		{
			mMaterial->mMaterialFlags |= (uint32_t)flag;
		}
		else
		{
			mMaterial->mMaterialFlags &= ~(uint32_t)flag;
		}
	}

	void MaterialInstance::MaterialValueUpdated(ShaderUniformDeclaration* decl)
	{
		if (mOverriddenValues.find(decl->GetName()) == mOverriddenValues.end())
		{
			auto& buffer = GetUniformBufferTarget(decl);
			auto& materialBuffer = mMaterial->GetUniformBufferTarget(decl);
			buffer.Write(materialBuffer.Data + decl->GetOffset(), decl->GetSize(), decl->GetOffset());
		}
	}

	Buffer& MaterialInstance::GetUniformBufferTarget(ShaderUniformDeclaration* uniformDeclaration)
	{
		switch (uniformDeclaration->GetDomain())
		{
		case ShaderDomain::Vertex:    return mVSUniformStorageBuffer;
		case ShaderDomain::Pixel:     return mPSUniformStorageBuffer;
		}

		NR_CORE_ASSERT(false, "Invalid uniform declaration domain! Material does not support this shader type.");
		return mVSUniformStorageBuffer;
	}

	void MaterialInstance::Bind()
	{
		mMaterial->mShader->Bind();

		if (mVSUniformStorageBuffer)
		{
			mMaterial->mShader->SetVSMaterialUniformBuffer(mVSUniformStorageBuffer);
		}

		if (mPSUniformStorageBuffer)
		{
			mMaterial->mShader->SetPSMaterialUniformBuffer(mPSUniformStorageBuffer);
		}

		mMaterial->BindTextures();
		for (size_t i = 0; i < mTextures.size(); ++i)
		{
			auto& texture = mTextures[i];
			if (texture)
			{
				texture->Bind(i);
			}
		}
	}
}