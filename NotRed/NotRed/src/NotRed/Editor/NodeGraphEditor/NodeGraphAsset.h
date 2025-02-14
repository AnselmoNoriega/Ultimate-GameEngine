#pragma once

#include "NotRed/Asset/Asset.h"

#include "NotRed/Editor/NodeGraphEditor/PropertySet.h"

#include "Nodes.h"

namespace NR
{
	class SOULSound : public Asset
	{
	public:
		std::vector<Node> Nodes;
		std::vector<Link> Links;
		std::string GraphState;

		Utils::PropertySet GraphInputs;
		Utils::PropertySet GraphOutputs;

		SOULSound() = default;

		static AssetType GetStaticType() { return AssetType::SOULSound; }
		AssetType GetAssetType() const override { return GetStaticType(); }
	};
}