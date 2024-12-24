#pragma once

#include "NotRed/Asset/Asset.h"
#include "NotRed/Renderer/Material.h"

#include <map>

namespace NR
{
	class MaterialAsset : public Asset
	{
	public:
		MaterialAsset();
		MaterialAsset(Ref<Material> material);
		~MaterialAsset() override = default;

		glm::vec3& GetAlbedoColor();
		void SetAlbedoColor(const glm::vec3& color);

		float& GetMetalness();
		void SetMetalness(float value);

		float& GetRoughness();
		void SetRoughness(float value);

		float& GetEmission();
		void SetEmission(float value);

		Ref<Texture2D> GetAlbedoMap();
		void SetAlbedoMap(Ref<Texture2D> texture);
		void ClearAlbedoMap();

		Ref<Texture2D> GetNormalMap();
		void SetNormalMap(Ref<Texture2D> texture);
		bool IsUsingNormalMap();
		void SetUseNormalMap(bool value);
		void ClearNormalMap();

		Ref<Texture2D> GetMetalnessMap();
		void SetMetalnessMap(Ref<Texture2D> texture);
		void ClearMetalnessMap();

		Ref<Texture2D> GetRoughnessMap();
		void SetRoughnessMap(Ref<Texture2D> texture);
		void ClearRoughnessMap();

		static AssetType GetStaticType() { return AssetType::Material; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		Ref<Material> GetMaterial() const { return mMaterial; }

	private:
		Ref<Material> mMaterial;
	};

	class MaterialTable : public RefCounted
	{
	public:
		MaterialTable(uint32_t materialCount = 1);
		MaterialTable(Ref<MaterialTable> other);
		~MaterialTable() = default;

		bool HasMaterial(uint32_t materialIndex) const { return mMaterials.find(materialIndex) != mMaterials.end(); }
		void SetMaterial(uint32_t index, Ref<MaterialAsset> material);
		void ClearMaterial(uint32_t index);

		Ref<MaterialAsset> GetMaterial(uint32_t materialIndex) const
		{
			NR_CORE_ASSERT(HasMaterial(materialIndex));
			return mMaterials.at(materialIndex);
		}

		std::map<uint32_t, Ref<MaterialAsset>>& GetMaterials() { return mMaterials; }
		const std::map<uint32_t, Ref<MaterialAsset>>& GetMaterials() const { return mMaterials; }

		uint32_t GetMaterialCount() const { return mMaterialCount; }
		void SetMaterialCount(uint32_t materialCount) { mMaterialCount = materialCount; }

		void Clear();

	private:
		std::map<uint32_t, Ref<MaterialAsset>> mMaterials;
		uint32_t mMaterialCount;
	};

}