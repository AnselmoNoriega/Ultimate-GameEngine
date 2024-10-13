#include "nrpch.h"
#include "MeshSerializer.h"

#include "yaml-cpp/yaml.h"
#include "NotRed/Asset/AssetManager.h"

namespace NR
{
	MeshSerializer::MeshSerializer()
	{
	}

	bool MeshSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		NR_CORE_ASSERT(false);
		return false;
	}

	void MeshSerializer::Serialize(const std::string& filepath)
	{
#if 0
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Mesh";
		{
			out << YAML::BeginMap;
			out << YAML::Key << "AssetHandle";
			out << YAML::Value << m_Mesh->Handle;
			out << YAML::Key << "MeshAsset";
			out << YAML::Value << m_Mesh->GetMeshAsset()->Handle;
			out << YAML::Key << "SubmeshIndices";
			out << YAML::Flow;
			out << YAML::Value << m_Mesh->GetSubmeshes();
			out << YAML::EndMap;
		}
		out << YAML::EndMap;
		NR_CORE_WARN("Serializing to {0}", filepath);
		std::ofstream fout(filepath);
		NR_CORE_ASSERT(fout.good());
		if (fout.good())
		{
			fout << out.c_str();
	}
#endif
	}

	void MeshSerializer::SerializeRuntime(const std::string& filepath)
	{
		NR_CORE_ASSERT(false);
	}

	bool MeshSerializer::Deserialize(const std::string& filepath)
	{
		return false;
	}

	bool MeshSerializer::DeserializeRuntime(const std::string& filepath)
	{
		NR_CORE_ASSERT(false);
		return false;
	}
}