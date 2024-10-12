#pragma once

#include "Texture.h"

namespace NR
{
	class Environment : public Asset
	{
	public:
		std::string FilePath;
		Ref<TextureCube> RadianceMap;
		Ref<TextureCube> IrradianceMap;

		Environment() = default;
		Environment(const Ref<TextureCube>& radianceMap, const Ref<TextureCube>& irradianceMap)
			: RadianceMap(radianceMap), IrradianceMap(irradianceMap) {}

		static AssetType GetStaticType() { return AssetType::EnvMap; }
		virtual AssetType GetAssetType() const override { return AssetType::EnvMap; }
	};
}