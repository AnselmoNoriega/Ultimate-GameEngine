#include "nrpch.h"
#include "MaterialAsset.h"

#include "NotRed/Renderer/Renderer.h"

namespace NR
{
	static const std::string sAlbedoColorUniform = "uMaterialUniforms.AlbedoColor";
	static const std::string sUseNormalMapUniform = "uMaterialUniforms.UseNormalMap";
	static const std::string sMetalnessUniform = "uMaterialUniforms.Metalness";
	static const std::string sRoughnessUniform = "uMaterialUniforms.Roughness";
	static const std::string sEmissionUniform = "uMaterialUniforms.Emission";

	static const std::string sAlbedoMapUniform = "uAlbedoTexture";
	static const std::string sNormalMapUniform = "uNormalTexture";
	static const std::string sMetalnessMapUniform = "uMetalnessTexture";
	static const std::string sRoughnessMapUniform = "uRoughnessTexture";

	MaterialAsset::MaterialAsset()
	{
		mMaterial = Material::Create(Renderer::GetShaderLibrary()->Get("PBR_Static"));

		// Defaults
		SetAlbedoColor(glm::vec3(0.8f));
		SetEmission(0.0f);
		SetUseNormalMap(false);
		SetMetalness(0.0f);
		SetRoughness(0.4f);

		// Maps
		SetAlbedoMap(Renderer::GetWhiteTexture());
		SetNormalMap(Renderer::GetWhiteTexture());
		SetMetalnessMap(Renderer::GetWhiteTexture());
		SetRoughnessMap(Renderer::GetWhiteTexture());
	}

	MaterialAsset::MaterialAsset(Ref<Material> material)
	{
		mMaterial = Material::Copy(material);
	}

	glm::vec3& MaterialAsset::GetAlbedoColor()
	{
		return mMaterial->GetVector3(sAlbedoColorUniform);
	}

	void MaterialAsset::SetAlbedoColor(const glm::vec3& color)
	{
		mMaterial->Set(sAlbedoColorUniform, color);
	}

	float& MaterialAsset::GetMetalness()
	{
		return mMaterial->GetFloat(sMetalnessUniform);
	}

	void MaterialAsset::SetMetalness(float value)
	{
		mMaterial->Set(sMetalnessUniform, value);
	}

	float& MaterialAsset::GetRoughness()
	{
		return mMaterial->GetFloat(sRoughnessUniform);
	}

	void MaterialAsset::SetRoughness(float value)
	{
		mMaterial->Set(sRoughnessUniform, value);
	}

	float& MaterialAsset::GetEmission()
	{
		return mMaterial->GetFloat(sEmissionUniform);
	}

	void MaterialAsset::SetEmission(float value)
	{
		mMaterial->Set(sEmissionUniform, value);
	}

	Ref<Texture2D> MaterialAsset::GetAlbedoMap()
	{
		return mMaterial->TryGetTexture2D(sAlbedoMapUniform);
	}

	void MaterialAsset::SetAlbedoMap(Ref<Texture2D> texture)
	{
		mMaterial->Set(sAlbedoMapUniform, texture);
	}

	void MaterialAsset::ClearAlbedoMap()
	{
		mMaterial->Set(sAlbedoMapUniform, Renderer::GetWhiteTexture());
	}

	Ref<Texture2D> MaterialAsset::GetNormalMap()
	{
		return mMaterial->TryGetTexture2D(sNormalMapUniform);
	}

	void MaterialAsset::SetNormalMap(Ref<Texture2D> texture)
	{
		mMaterial->Set(sNormalMapUniform, texture);
	}

	bool MaterialAsset::IsUsingNormalMap()
	{
		return mMaterial->GetBool(sUseNormalMapUniform);
	}

	void MaterialAsset::SetUseNormalMap(bool value)
	{
		mMaterial->Set(sUseNormalMapUniform, value);
	}

	void MaterialAsset::ClearNormalMap()
	{
		mMaterial->Set(sNormalMapUniform, Renderer::GetWhiteTexture());
	}

	Ref<Texture2D> MaterialAsset::GetMetalnessMap()
	{
		return mMaterial->TryGetTexture2D(sMetalnessMapUniform);
	}

	void MaterialAsset::SetMetalnessMap(Ref<Texture2D> texture)
	{
		mMaterial->Set(sMetalnessMapUniform, texture);
	}

	void MaterialAsset::ClearMetalnessMap()
	{
		mMaterial->Set(sMetalnessMapUniform, Renderer::GetWhiteTexture());
	}

	Ref<Texture2D> MaterialAsset::GetRoughnessMap()
	{
		return mMaterial->TryGetTexture2D(sRoughnessMapUniform);
	}

	void MaterialAsset::SetRoughnessMap(Ref<Texture2D> texture)
	{
		mMaterial->Set(sRoughnessMapUniform, texture);
	}

	void MaterialAsset::ClearRoughnessMap()
	{
		mMaterial->Set(sRoughnessMapUniform, Renderer::GetWhiteTexture());
	}

	MaterialTable::MaterialTable(uint32_t materialCount)
		: mMaterialCount(materialCount)
	{
	}

	MaterialTable::MaterialTable(Ref<MaterialTable> other)
	{
		const auto& meshMaterials = other->GetMaterials();
		for (auto [index, materialAsset] : meshMaterials)
		{
			SetMaterial(index, Ref<MaterialAsset>::Create(materialAsset->GetMaterial()));
		}
	}

	void MaterialTable::SetMaterial(uint32_t index, Ref<MaterialAsset> material)
	{
		mMaterials[index] = material;
		if (index >= mMaterialCount)
		{
			mMaterialCount = index + 1;
		}
	}

	void MaterialTable::ClearMaterial(uint32_t index)
	{
		NR_CORE_ASSERT(HasMaterial(index));
		mMaterials.erase(index);
		if (index >= mMaterialCount)
		{
			mMaterialCount = index + 1;
		}
	}

	void MaterialTable::Clear()
	{
		mMaterials.clear();
	}

}