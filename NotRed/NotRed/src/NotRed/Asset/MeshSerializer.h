#pragma once

#include "NotRed/Asset/AssetSerializer.h"
#include "NotRed/Renderer/Mesh.h"

namespace NR
{
	class MeshSerializer : public AssetSerializer
	{
	public:
		MeshSerializer();

		void Serialize(const std::string& filepath);
		void SerializeRuntime(const std::string& filepath);

		bool Deserialize(const std::string& filepath);
		bool DeserializeRuntime(const std::string& filepath);

		void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override {}
		bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
	};
}