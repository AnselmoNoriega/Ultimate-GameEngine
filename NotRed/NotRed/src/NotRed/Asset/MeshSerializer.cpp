#include "nrpch.h"
#include "MeshSerializer.h"

#include "yaml-cpp/yaml.h"
#include "NotRed/Asset/AssetManager.h"

namespace YAML
{
    template<>
    struct convert<std::vector<uint32_t>>
    {
        static Node encode(const std::vector<uint32_t>& value)
        {
            Node node;
            for (uint32_t element : value)
            {
                node.push_back(element);
            }

            return node;
        }

        static bool decode(const Node& node, std::vector<uint32_t>& result)
        {
            if (!node.IsSequence())
            {
                return false;
            }

            result.resize(node.size());
            for (size_t i = 0; i < node.size(); ++i)
            {
                result[i] = node[i].as<uint32_t>();
            }

            return true;
        }
    };
}

namespace NR
{
    YAML::Emitter& operator<<(YAML::Emitter& out, const std::vector<uint32_t>& value)
    {
        out << YAML::Flow;
        out << YAML::BeginSeq;
        for (uint32_t element : value)
        {
            out << element;
        }
        out << YAML::EndSeq;
        return out;
    }

    MeshSerializer::MeshSerializer()
    {
    }

    bool MeshSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
    {
        std::ifstream stream(metadata.FilePath);
        NR_CORE_ASSERT(stream);

        std::stringstream strStream;
        strStream << stream.rdbuf();

        YAML::Node data = YAML::Load(strStream.str());
        if (!data["Mesh"])
        {
            return false;
        }

        YAML::Node rootNode = data["Mesh"];
        if (!rootNode["MeshAsset"])
        {
            return false;
        }

        AssetHandle assetHandle = rootNode["MeshAsset"].as<uint64_t>();
        Ref<MeshAsset> meshAsset = AssetManager::GetAsset<MeshAsset>(assetHandle);
        auto submeshIndices = rootNode["SubmeshIndices"].as<std::vector<uint32_t>>();

        Ref<Mesh> mesh = Ref<Mesh>::Create(meshAsset, submeshIndices);
        mesh->Handle = metadata.Handle;
        asset = mesh;

        return true;
    }

    void MeshSerializer::Serialize(Ref<Mesh> mesh, const std::string& filepath)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Mesh";
        {
            out << YAML::BeginMap;
            out << YAML::Key << "AssetHandle";
            out << YAML::Value << mesh->Handle;
            out << YAML::Key << "MeshAsset";
            out << YAML::Value << mesh->GetMeshAsset()->Handle;
            out << YAML::Key << "SubmeshIndices";
            out << YAML::Flow;
            out << YAML::Value << mesh->GetSubmeshes();
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
    }

    void MeshSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
    {
        MeshSerializer serializer;
        serializer.Serialize(asset.As<Mesh>(), metadata.FilePath);
    }

    void MeshSerializer::SerializeRuntime(Ref<Mesh> mesh, const std::string& filepath)
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