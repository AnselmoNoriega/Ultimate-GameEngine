#pragma once

#include "NotRed/Asset/Asset.h"

#include "Nodes.h"

namespace NR
{
	class DemoGraph : public Asset
	{
	public:
		std::vector<Node> Nodes;
		std::vector<Link> Links;

		DemoGraph() = default;

		static AssetType GetStaticType() { return AssetType::DemoGraph; }
		AssetType GetAssetType() const override { return GetStaticType(); }
	};
}