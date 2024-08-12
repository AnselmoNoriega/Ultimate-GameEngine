#include "nrpch.h"
#include "AssetLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "NotRed/Renderer/Mesh.h"

#include "NotRed/Renderer/Renderer.h"
#include "NotRed/Renderer/VertexArray.h"
#include "NotRed/Renderer/VertexBuffer.h"
#include "NotRed/Renderer/IndexBuffer.h"
#include "NotRed/Renderer/Texture.h"

namespace NR
{
    bool AssetLoader::LoadModel(const std::string& filePath)
    {
        static std::filesystem::path modelsDirectory = std::filesystem::current_path().parent_path() /
            "Sandbox" / "Assets" / "Models";

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile((modelsDirectory / filePath).string(), aiProcess_Triangulate | aiProcess_FlipUVs);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            return false;
        }

        ProcessNode(scene->mRootNode, scene);

        return true;
    }

    void AssetLoader::ProcessNode(aiNode* node, const aiScene* scene)
    {
        for (int i = 0; i < node->mNumMeshes; ++i)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            ProcessMesh(mesh, scene);
        }

        for (int i = 0; i < node->mNumChildren; ++i)
        {
            ProcessNode(node->mChildren[i], scene);
        }
    }

    Ref<Mesh> AssetLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene)
    {
        Ref<Mesh> model;

        std::vector<Ref<Texture2D>> diffuseMaps;
        //model.meshData.reserve(numMaterials);
        for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
        {
            const aiMaterial* assimpMaterial = scene->mMaterials[i];
            aiColor3D ambient, diffuse, emissive, specular;
            ai_real specularPower = 1.0f;

            assimpMaterial->Get(AI_MATKEY_COLOR_AMBIENT, ambient);
            assimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
            assimpMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, emissive);
            assimpMaterial->Get(AI_MATKEY_COLOR_SPECULAR, specular);
            assimpMaterial->Get(AI_MATKEY_SHININESS, specularPower);

            Model::MeterialData& materialData = model.meterialData.emplace_back();
            materialData.material.ambient = ToColor(ambient);
            materialData.material.diffuse = ToColor(diffuse);
            materialData.material.emissive = ToColor(emissive);
            materialData.material.specular = ToColor(specular);
            materialData.material.power = (float)specularPower;

            materialData.diffuseMapName = FindTexture(scene, assimpMaterial, aiTextureType_DIFFUSE, args, "_diff", materialIndex);
            materialData.normalMapName = FindTexture(scene, assimpMaterial, aiTextureType_NORMALS, args, "_norm", materialIndex);
            materialData.bumpMapName = FindTexture(scene, assimpMaterial, aiTextureType_DISPLACEMENT, args, "_bump", materialIndex);
            materialData.specularMapName = FindTexture(scene, assimpMaterial, aiTextureType_SPECULAR, args, "_spec", materialIndex);
        }
        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            for (int i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); ++i)
            {
                aiString str;
                material->GetTexture(aiTextureType_DIFFUSE, i, &str);
                std::string fullPath = "Assets/Models/" + std::string(str.C_Str());
                Ref<Texture2D> texture = Texture2D::Create(fullPath);
                diffuseMaps.push_back(texture);
            }
        }

        {
            Ref<VertexArray> vertexArray;
            Ref<VertexBuffer> vertexBuffer;
            Renderer::SetMeshLayout(vertexArray, vertexBuffer, mesh->mNumVertices);

            std::vector<uint32_t> indices;
            for (int i = 0; i < mesh->mNumFaces; i++)
            {
                aiFace face = mesh->mFaces[i];
                for (int j = 0; j < face.mNumIndices; j++)
                {
                    indices.push_back(face.mIndices[j]);
                }
            }
            Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(indices.data(), indices.size());
            vertexArray->SetIndexBuffer(indexBuffer);

            model = CreateRef<Mesh>(vertexArray, vertexBuffer, diffuseMaps, mesh->mNumVertices);
        }

        for (int i = 0; i < mesh->mNumVertices; ++i)
        {
            glm::vec3 position;

            position.x = mesh->mVertices[i].x;
            position.y = mesh->mVertices[i].y;
            position.z = mesh->mVertices[i].z;

            //if (mesh->HasNormals())
            //{
            //	vector.x = mesh->mNormals[i].x;
            //	vector.y = mesh->mNormals[i].y;
            //	vector.z = mesh->mNormals[i].z;
            //}

            glm::vec2 texCoord = glm::vec2(0.0f, 0.0f);
            if (mesh->mTextureCoords[0])
            {
                texCoord.x = mesh->mTextureCoords[0][i].x;
                texCoord.y = mesh->mTextureCoords[0][i].y;
            }

            Renderer::PackageVertices(model, position, texCoord);
        }

        // Process material (textures)

        // Return a mesh object created from the extracted mesh data
        return model;
    }
}