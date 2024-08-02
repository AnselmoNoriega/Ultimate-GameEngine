#pragma once

struct aiNode;
struct aiScene;
struct aiMesh;

namespace NR
{
    class Mesh;

    class AssetLoader
    {
    public:
        static bool LoadModel(const std::string& filePath);

    private:
        static void ProcessNode(aiNode* node, const aiScene* scene);
        static Ref<Mesh> ProcessMesh(aiMesh* mesh, const aiScene* scene);
    };
}

