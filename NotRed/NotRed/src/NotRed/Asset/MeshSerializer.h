#pragma once

#include "NotRed/Asset/AssetSerializer.h"
#include "NotRed/Renderer/Mesh.h"

namespace Hazel {
	class MeshSerializer : public AssetSerializer
	{
	public:
		MeshSerializer();

		void Serialize(const std::string& filepath);
		void SerializeRuntime(const std::string& filepath);

		bool Deserialize(const std::string& filepath);
		bool DeserializeRuntime(const std::string& filepath);

		virtual void Serialize(const Ref<Asset>& asset) const override {}
		virtual bool TryLoadData(Ref<Asset>& asset) const override;
	};
}