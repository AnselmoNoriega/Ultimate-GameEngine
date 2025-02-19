#include "nrpch.h"
#include "MeshSerializer.h"

#include "yaml-cpp/yaml.h"
#include "NotRed/Asset/AssetManager.h"
#include "NotRed/Project/Project.h"

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
        auto filepath = Project::GetAssetDirectory() / metadata.FilePath;
        std::ifstream stream(filepath);
        NR_CORE_ASSERT(stream);

        std::stringstream strStream;
        strStream << stream.rdbuf();

        YAML::Node data = YAML::Load(strStream.str());
        if (!data["Mesh"])
        {
            return false;
        }

        YAML::Node rootNode = data["Mesh"];
        if (!rootNode["MeshAsset"] && !rootNode["MeshSource"])
            return false;

        AssetHandle meshSourceHandle = 0;
        if (rootNode["MeshAsset"]) // DEPRECATED
        {
            meshSourceHandle = rootNode["MeshAsset"].as<uint64_t>();
        }
        else
        {
            meshSourceHandle = rootNode["MeshSource"].as<uint64_t>();
        }

        Ref<MeshSource> meshAsset = AssetManager::GetAsset<MeshSource>(meshSourceHandle);
        if (!meshAsset)
        {
            return false;
        }

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
            out << YAML::Key << "MeshSource";
            out << YAML::Value << mesh->GetMeshSource()->Handle;
            out << YAML::Key << "SubmeshIndices";
            out << YAML::Flow;
            if (mesh->GetSubmeshes().size() == mesh->GetMeshSource()->GetSubmeshes().size())
            {
                out << YAML::Value << std::vector<uint32_t>();
            }
            else
            {
                out << YAML::Value << mesh->GetSubmeshes();
            }
            out << YAML::EndMap;
        }
        out << YAML::EndMap;

        auto serializePath = Project::GetActive()->GetAssetDirectory() / filepath;
        NR_CORE_WARN("Serializing to {0}", serializePath.string());
        std::ofstream fout(serializePath);

        NR_CORE_ASSERT(fout.good());

        if (fout.good())
        {
            fout << out.c_str();
        }
    }

    void MeshSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
    {
        MeshSerializer serializer;
        serializer.Serialize(asset.As<Mesh>(), metadata.FilePath.string());
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

    StaticMeshSerializer::StaticMeshSerializer()
    {
    }

    bool StaticMeshSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
    {
        auto filepath = Project::GetAssetDirectory() / metadata.FilePath;
        std::ifstream stream(filepath);
        NR_CORE_ASSERT(stream);

        std::stringstream strStream;
        strStream << stream.rdbuf();
        YAML::Node data = YAML::Load(strStream.str());
        if (!data["Mesh"])
        {
            return false;
        }

        YAML::Node rootNode = data["Mesh"];
        if (!rootNode["MeshAsset"] && !rootNode["MeshSource"])
        {
            return false;
        }

        AssetHandle meshSourceHandle = rootNode["MeshSource"].as<uint64_t>();
        Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(meshSourceHandle);
        if (!meshSource)
        {
            return false; // TODO(Yan): feedback to the user
        }

        auto submeshIndices = rootNode["SubmeshIndices"].as<std::vector<uint32_t>>();
        Ref<StaticMesh> mesh = Ref<StaticMesh>::Create(meshSource, submeshIndices);
        mesh->Handle = metadata.Handle;
        asset = mesh;

        return true;
    }

    void StaticMeshSerializer::Serialize(Ref<StaticMesh> mesh, const std::string& filepath)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Mesh";
        {
            out << YAML::BeginMap;
            out << YAML::Key << "MeshSource";
            out << YAML::Value << mesh->GetMeshSource()->Handle;
            out << YAML::Key << "SubmeshIndices";
            out << YAML::Flow;
            if (mesh->GetSubmeshes().size() == mesh->GetMeshSource()->GetSubmeshes().size())
                out << YAML::Value << std::vector<uint32_t>();
            else
                out << YAML::Value << mesh->GetSubmeshes();
            out << YAML::EndMap;
        }
        out << YAML::EndMap;

        auto serializePath = Project::GetActive()->GetAssetDirectory() / filepath;
        NR_CORE_WARN("Serializing to {0}", serializePath.string());
        
        std::ofstream fout(serializePath);
        NR_CORE_ASSERT(fout.good());
        
        if (fout.good())
        {
            fout << out.c_str();
        }
    }

    void StaticMeshSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
    {
        StaticMeshSerializer serializer;
        serializer.Serialize(asset.As<Mesh>(), metadata.FilePath.string());
    }

    void StaticMeshSerializer::SerializeRuntime(Ref<StaticMesh> mesh, const std::string& filepath)
    {
        NR_CORE_ASSERT(false);
    }

    bool StaticMeshSerializer::Deserialize(const std::string& filepath)
    {
        return false;
    }

    bool StaticMeshSerializer::DeserializeRuntime(const std::string& filepath)
    {
        NR_CORE_ASSERT(false);
        return false;
    }
}