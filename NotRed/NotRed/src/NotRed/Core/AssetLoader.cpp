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

namespace NR
{
    bool AssetLoader::LoadModel(const std::string& filePath)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            return false;
        }

        ProcessNode(scene->mRootNode, scene);

        return true;
    }

    void AssetLoader::ProcessNode(aiNode* node, const aiScene* scene)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; ++i)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            ProcessMesh(mesh, scene);
        }

        for (unsigned int i = 0; i < node->mNumChildren; ++i)
        {
            ProcessNode(node->mChildren[i], scene);
        }
    }

    Ref<Mesh> AssetLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene)
    {
        Ref<VertexArray> vertexArray;
        Ref<VertexBuffer> vertexBuffer;

        Renderer::SetMeshLayout(vertexArray, vertexBuffer, mesh->mNumVertices);

        std::vector<uint32_t> indices;
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
            {
                indices.push_back(face.mIndices[j]);
            }
        }
        Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(indices.data(), indices.size());
        vertexArray->SetIndexBuffer(indexBuffer);
        
        std::vector<Vertex> vertices;
        std::vector<Texture> textures;

        mesh->mNumVertices;

        for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
        {
            Vertex vertex;
            glm::vec3 vector;

            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;

            if (mesh->HasNormals()) 
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }

            if (mesh->mTextureCoords[0]) 
            {
                glm::vec2 vec;
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
            }
            else 
            {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }

            vertices.push_back(vertex);
        }

        // Process material (textures)
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            std::vector<Texture> diffuseMaps = LoadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        }

        // Return a mesh object created from the extracted mesh data
        return Mesh(vertices, indices, textures);
    }
}