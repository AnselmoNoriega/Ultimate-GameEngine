#pragma once

#include "NotRed/Asset/AssetSerializer.h"
#include "NotRed/Editor/NodeGraphEditor/Nodes.h"
#include "yaml-cpp/yaml.h"

namespace NR
{
	class DefaultGraphSerializer : public AssetSerializer
	{
	public:
		void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override;
		bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
		
		static void SerializeNodes(YAML::Emitter& out, const std::vector<Node>& nodes);
		static void SerializeLinks(YAML::Emitter& out, const std::vector<Link>& links);
		static void TryLoadNodes(YAML::Node& in, std::vector<Node>& nodes);
		static void TryLoadLinks(YAML::Node& in, std::vector<Link>& links);
	};

	class SOULGraphSerializer : public AssetSerializer
	{
	public:
		void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override;
		bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
	};
}