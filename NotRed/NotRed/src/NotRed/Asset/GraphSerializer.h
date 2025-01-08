#pragma once

#include "NotRed/Asset/AssetSerializer.h"
#include "NotRed/Renderer/Mesh.h"

namespace NR
{
	class DefaultGraphSerializer : public AssetSerializer
	{
	public:
		void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override;
		bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
	};
}