#include "nrpch.h"
#include "Material.h"

namespace NR
{
	Material::Material(const Ref<Shader>& shader)
		: mShader(shader)
	{
		mShader->AddShaderReloadedCallback(std::bind(&Material::OnShaderReloaded, this));
		AllocateStorage();
	}

	Material::~Material()
	{
	}

	void Material::AllocateStorage()
	{
		const auto& vsBuffer = mShader->GetVSMaterialUniformBuffer();
		mVSUniformStorageBuffer.Allocate(vsBuffer.GetSize());
		mVSUniformStorageBuffer.ZeroInitialize();

		const auto& psBuffer = mShader->GetPSMaterialUniformBuffer();
		mPSUniformStorageBuffer.Allocate(psBuffer.GetSize());
		mPSUniformStorageBuffer.ZeroInitialize();
	}

	void Material::OnShaderReloaded()
	{
		AllocateStorage();

		for (auto mi : mMaterialInstances)
			mi->OnShaderReloaded();
	}

	ShaderUniformDeclaration* Material::FindUniformDeclaration(const std::string& name)
	{
		if (mVSUniformStorageBuffer)
		{
			auto& declarations = mShader->GetVSMaterialUniformBuffer().GetUniformDeclarations();
			for (ShaderUniformDeclaration* uniform : declarations)
			{
				if (uniform->GetName() == name)
					return uniform;
			}
		}

		if (mPSUniformStorageBuffer)
		{
			auto& declarations = mShader->GetPSMaterialUniformBuffer().GetUniformDeclarations();
			for (ShaderUniformDeclaration* uniform : declarations)
			{
				if (uniform->GetName() == name)
					return uniform;
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
				return resource;
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

		HZ_CORE_ASSERT(false, "Invalid uniform declaration domain! Material does not support this shader type.");
		return mVSUniformStorageBuffer;
	}

	void Material::Bind() const
	{
		mShader->Bind();

		if (mVSUniformStorageBuffer)
			mShader->SetVSMaterialUniformBuffer(mVSUniformStorageBuffer);

		if (mPSUniformStorageBuffer)
			mShader->SetPSMaterialUniformBuffer(mPSUniformStorageBuffer);

		BindTextures();
	}

	void Material::BindTextures() const
	{
		for (size_t i = 0; i < mTextures.size(); i++)
		{
			auto& texture = mTextures[i];
			if (texture)
				texture->Bind(i);
		}
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

	void MaterialInstance::OnShaderReloaded()
	{
		AllocateStorage();
		mOverriddenValues.clear();
	}

	void MaterialInstance::AllocateStorage()
	{
		const auto& vsBuffer = mMaterial->mShader->GetVSMaterialUniformBuffer();
		mVSUniformStorageBuffer.Allocate(vsBuffer.GetSize());
		memcpy(mVSUniformStorageBuffer.Data, mMaterial->mVSUniformStorageBuffer.Data, vsBuffer.GetSize());

		const auto& psBuffer = mMaterial->mShader->GetPSMaterialUniformBuffer();
		mPSUniformStorageBuffer.Allocate(psBuffer.GetSize());
		memcpy(mPSUniformStorageBuffer.Data, mMaterial->mPSUniformStorageBuffer.Data, psBuffer.GetSize());
	}

	void MaterialInstance::OnMaterialValueUpdated(ShaderUniformDeclaration* decl)
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

		HZ_CORE_ASSERT(false, "Invalid uniform declaration domain! Material does not support this shader type.");
		return mVSUniformStorageBuffer;
	}

	void MaterialInstance::Bind() const
	{
		if (mVSUniformStorageBuffer)
			mMaterial->mShader->SetVSMaterialUniformBuffer(mVSUniformStorageBuffer);

		if (mPSUniformStorageBuffer)
			mMaterial->mShader->SetPSMaterialUniformBuffer(mPSUniformStorageBuffer);

		mMaterial->BindTextures();
		for (size_t i = 0; i < mTextures.size(); i++)
		{
			auto& texture = mTextures[i];
			if (texture)
				texture->Bind(i);
		}
	}
}