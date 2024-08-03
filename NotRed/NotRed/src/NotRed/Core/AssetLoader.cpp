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
		Ref<Mesh> model;
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

			model = CreateRef<Mesh>(vertexArray, vertexBuffer);
		}

		for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
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

			glm::vec2 vec = glm::vec2(0.0f, 0.0f);
			if (mesh->mTextureCoords[0])
			{
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
			}
		}

		// Process material (textures)
		//if (mesh->mMaterialIndex >= 0) {
		//	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		//	std::vector<Texture> diffuseMaps = LoadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
		//	textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		//}

		// Return a mesh object created from the extracted mesh data
		return Mesh(vertices, indices, textures);
	}
}