#include "nrpch.h" 
#include "Mesh.h"

#include <filesystem>
#include <glad/glad.h>

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLmENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>

#include "imgui/imgui.h"

#include "NotRed/Debug/Profiler.h"

#include "NotRed/Renderer/Renderer.h"
#include "NotRed/Renderer/VertexBuffer.h"

namespace NR
{

#define MESH_DEBUG_LOG 1
#if MESH_DEBUG_LOG
#define NR_MESH_LOG(...) NR_CORE_TRACE(__VA_ARGS__)
#else
#define NR_MESH_LOG(...)
#endif

	static const uint32_t sMeshImportFlags =
		aiProcess_CalcTangentSpace |        // Create binormals/tangents
		aiProcess_Triangulate |             // Make sure we're triangles
		aiProcess_SortByPType |             // Split meshes by primitive type
		aiProcess_GenNormals |              // Make sure we have normals
		aiProcess_GenUVCoords |             // Convert UVs if required 
		aiProcess_OptimizeMeshes |          // Batch draws where possible
		aiProcess_JoinIdenticalVertices |   // Remove vertices that are identical
		aiProcess_LimitBoneWeights |        // If more than 4 bone weights, discard least influencing ones and renormalize sum to 1
		aiProcess_ValidateDataStructure;    // Validation

	struct LogStream : public Assimp::LogStream
	{
		static void Initialize()
		{
			if (Assimp::DefaultLogger::isNullLogger())
			{
				Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
				Assimp::DefaultLogger::get()->attachStream(new LogStream, Assimp::Logger::Err | Assimp::Logger::Warn);
			}
		}

		virtual void write(const char* message) override
		{
			NR_CORE_ERROR("Assimp error: {0}", message);
		}
	};

	static std::string LevelToSpaces(uint32_t level)
	{
		std::string result = "";
		for (uint32_t i = 0; i < level; ++i)
		{
			result += "--";
		}
		return result;
	}

	glm::mat4 AssimpMat4ToMat4(const aiMatrix4x4& matrix)
	{
		glm::mat4 result;
		//the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
		result[0][0] = matrix.a1; result[1][0] = matrix.a2; result[2][0] = matrix.a3; result[3][0] = matrix.a4;
		result[0][1] = matrix.b1; result[1][1] = matrix.b2; result[2][1] = matrix.b3; result[3][1] = matrix.b4;
		result[0][2] = matrix.c1; result[1][2] = matrix.c2; result[2][2] = matrix.c3; result[3][2] = matrix.c4;
		result[0][3] = matrix.d1; result[1][3] = matrix.d2; result[2][3] = matrix.d3; result[3][3] = matrix.d4;
		return result;
	}

	MeshAsset::MeshAsset(const std::string& filename)
		: mFilePath(filename)
	{
		LogStream::Initialize();

		NR_MESH_LOG("Loading mesh: {0}", filename.c_str());

		mImporter = std::make_unique<Assimp::Importer>();

		std::filesystem::path modelsDirectory = std::filesystem::current_path().parent_path() /
			"NotEditor" / filename;

		const aiScene* scene = mImporter->ReadFile(modelsDirectory.string(), sMeshImportFlags);
		if (!scene || !scene->HasMeshes())
		{
			NR_CORE_ERROR("Failed to load mesh file: {0}", modelsDirectory.string());
			ModifyFlags(AssetFlag::Invalid);
			return;
		}

		mScene = scene;

		mIsAnimated = scene->mAnimations != nullptr;
		mInverseTransform = glm::inverse(AssimpMat4ToMat4(scene->mRootNode->mTransformation));

		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;

		mBoundingBox.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
		mBoundingBox.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

		mSubmeshes.reserve(scene->mNumMeshes);
		for (unsigned m = 0; m < scene->mNumMeshes; ++m)
		{
			aiMesh* mesh = scene->mMeshes[m];

			Submesh& submesh = mSubmeshes.emplace_back();
			submesh.BaseVertex = vertexCount;
			submesh.BaseIndex = indexCount;
			submesh.VertexCount = mesh->mNumVertices;
			submesh.IndexCount = mesh->mNumFaces * 3;
			submesh.MaterialIndex = mesh->mMaterialIndex;
			submesh.MeshName = mesh->mName.C_Str();

			vertexCount += mesh->mNumVertices;
			indexCount += submesh.IndexCount;

			NR_CORE_ASSERT(mesh->HasPositions(), "Meshes require positions.");
			NR_CORE_ASSERT(mesh->HasNormals(), "Meshes require normals.");

			auto& aabb = submesh.BoundingBox;
			aabb.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
			aabb.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
			for (size_t i = 0; i < mesh->mNumVertices; i++)
			{
				Vertex vertex;
				vertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
				vertex.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
				aabb.Min.x = glm::min(vertex.Position.x, aabb.Min.x);
				aabb.Min.y = glm::min(vertex.Position.y, aabb.Min.y);
				aabb.Min.z = glm::min(vertex.Position.z, aabb.Min.z);
				aabb.Max.x = glm::max(vertex.Position.x, aabb.Max.x);
				aabb.Max.y = glm::max(vertex.Position.y, aabb.Max.y);
				aabb.Max.z = glm::max(vertex.Position.z, aabb.Max.z);

				if (mesh->HasTangentsAndBitangents())
				{
					vertex.Tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
					vertex.Binormal = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
				}

				if (mesh->HasTextureCoords(0))
				{
					vertex.Texcoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
				}

				if (mIsAnimated)
				{
					mStaticVertices.push_back(vertex);
					mAnimatedVertices.push_back(vertex);
				}
				else
				{
					mStaticVertices.push_back(vertex);
				}
			}

			for (size_t i = 0; i < mesh->mNumFaces; ++i)
			{
				NR_CORE_ASSERT(mesh->mFaces[i].mNumIndices == 3, "Must have 3 indices.");
				Index index = { mesh->mFaces[i].mIndices[0], mesh->mFaces[i].mIndices[1], mesh->mFaces[i].mIndices[2] };
				mIndices.push_back(index);

				mTriangleCache[m].emplace_back(mStaticVertices[index.V1 + submesh.BaseVertex], mStaticVertices[index.V2 + submesh.BaseVertex], mStaticVertices[index.V3 + submesh.BaseVertex]);
			}
		}

		TraverseNodes(scene->mRootNode);

		for (const auto& submesh : mSubmeshes)
		{
			AABB transformedSubmeshAABB = submesh.BoundingBox;
			glm::vec3 min = glm::vec3(submesh.Transform * glm::vec4(transformedSubmeshAABB.Min, 1.0f));
			glm::vec3 max = glm::vec3(submesh.Transform * glm::vec4(transformedSubmeshAABB.Max, 1.0f));
			mBoundingBox.Min.x = glm::min(mBoundingBox.Min.x, min.x);
			mBoundingBox.Min.y = glm::min(mBoundingBox.Min.y, min.y);
			mBoundingBox.Min.z = glm::min(mBoundingBox.Min.z, min.z);
			mBoundingBox.Max.x = glm::max(mBoundingBox.Max.x, max.x);
			mBoundingBox.Max.y = glm::max(mBoundingBox.Max.y, max.y);
			mBoundingBox.Max.z = glm::max(mBoundingBox.Max.z, max.z);
		}

		// Bones
		if (mIsAnimated)
		{
			for (size_t m = 0; m < scene->mNumMeshes; ++m)
			{
				aiMesh* mesh = scene->mMeshes[m];
				Submesh& submesh = mSubmeshes[m];
				for (size_t i = 0; i < mesh->mNumBones; ++i)
				{
					aiBone* bone = mesh->mBones[i];
					std::string boneName(bone->mName.data);
					int boneIndex = 0;
					if (mBoneMapping.find(boneName) == mBoneMapping.end())
					{
						// Allocate an index for a new bone
						boneIndex = mBoneCount;
						++mBoneCount;
						BoneInfo bi;
						mBoneInfo.push_back(bi);
						mBoneInfo[boneIndex].BoneOffset = AssimpMat4ToMat4(bone->mOffsetMatrix);
						mBoneMapping[boneName] = boneIndex;
					}
					else
					{
						NR_MESH_LOG("Found existing bone in map");
						boneIndex = mBoneMapping[boneName];
					}
					for (size_t j = 0; j < bone->mNumWeights; j++)
					{
						int VertexID = submesh.BaseVertex + bone->mWeights[j].mVertexId;
						float Weight = bone->mWeights[j].mWeight;
						mAnimatedVertices[VertexID].AddBoneData(boneIndex, Weight);
					}
				}
			}
		}

		Ref<Texture2D> whiteTexture = Renderer::GetWhiteTexture();
		if (scene->HasMaterials())
		{
			mMaterials.resize(scene->mNumMaterials);

			for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
			{
				auto aiMaterial = scene->mMaterials[i];
				auto aiMaterialName = aiMaterial->GetName();

				Ref<Material> mi = Material::Create(Renderer::GetShaderLibrary()->Get("PBR_Static"), aiMaterialName.data);
				mMaterials[i] = mi;

				NR_MESH_LOG("Material Name = {0}; Index = {1}", aiMaterialName.data, i);
				aiString aiTexPath;
				uint32_t textureCount = aiMaterial->GetTextureCount(aiTextureType_DIFFUSE);
				NR_MESH_LOG("  TextureCount = {0}", textureCount);

				glm::vec3 albedoColor(0.8f);
				float emission = 0.0f;
				aiColor3D aiColor, aiEmission;
				if (aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiColor) == AI_SUCCESS)
				{
					albedoColor = { aiColor.r, aiColor.g, aiColor.b };
				}

				if (aiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, aiEmission) == AI_SUCCESS)
				{
					emission = aiEmission.r;
				}

				mi->Set("uMaterialUniforms.AlbedoColor", albedoColor);
				mi->Set("uMaterialUniforms.Emission", emission);

				float shininess, metalness;
				if (aiMaterial->Get(AI_MATKEY_SHININESS, shininess) != aiReturn_SUCCESS)
				{
					shininess = 80.0f;
				}

				if (aiMaterial->Get(AI_MATKEY_REFLECTIVITY, metalness) != aiReturn_SUCCESS)
				{
					metalness = 0.0f;
				}

				float roughness = 1.0f - glm::sqrt(shininess / 100.0f);
				NR_MESH_LOG("    COLOR = {0}, {1}, {2}", aiColor.r, aiColor.g, aiColor.b);
				NR_MESH_LOG("    ROUGHNESS = {0}", roughness);
				NR_MESH_LOG("    METALNESS = {0}", metalness);

				bool hasAlbedoMap = aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &aiTexPath) == AI_SUCCESS;
				bool fallback = !hasAlbedoMap;
				if (hasAlbedoMap)
				{
					std::filesystem::path path = modelsDirectory;
					auto parentPath = path.parent_path();
					parentPath /= std::string(aiTexPath.data);
					std::string texturePath = parentPath.string();

					NR_MESH_LOG("    Albedo map path = {0}", texturePath);
					TextureProperties props;
					props.StandardRGB = true;
					auto texture = Texture2D::Create(texturePath, props);
					if (texture->Loaded())
					{
						mi->Set("uAlbedoTexture", texture);
						mi->Set("uMaterialUniforms.AlbedoColor", glm::vec3(1.0f));
					}
					else
					{
						NR_CORE_ERROR("Could not load texture: {0}", texturePath);
						fallback = true;
					}
				}

				if (fallback)
				{
					NR_MESH_LOG("    No albedo map");
					mi->Set("uAlbedoTexture", whiteTexture);
				}

				// Normal maps
				bool hasNormalMap = aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &aiTexPath) == AI_SUCCESS;
				fallback = !hasNormalMap;
				if (hasNormalMap)
				{
					std::filesystem::path path = filename;
					auto parentPath = path.parent_path();
					parentPath /= std::string(aiTexPath.data);
					std::string texturePath = parentPath.string();

					NR_MESH_LOG("    Normal map path = {0}", texturePath);
					auto texture = Texture2D::Create(texturePath);
					if (texture->Loaded())
					{
						mi->Set("uNormalTexture", texture);
						mi->Set("uMaterialUniforms.UseNormalMap", true);
					}
					else
					{
						NR_CORE_ERROR("  Could not load normal map");
						fallback = true;
					}
				}

				if (fallback)
				{
					NR_MESH_LOG("   Mesh has no normal map");
					mi->Set("uNormalTexture", whiteTexture);
					mi->Set("uMaterialUniforms.UseNormalMap", false);
				}

				// Roughness map
				bool hasRoughnessMap = aiMaterial->GetTexture(aiTextureType_SHININESS, 0, &aiTexPath) == AI_SUCCESS;
				fallback = !hasRoughnessMap;
				if (hasRoughnessMap)
				{
					std::filesystem::path path = filename;
					auto parentPath = path.parent_path();
					parentPath /= std::string(aiTexPath.data);
					std::string texturePath = parentPath.string();

					NR_MESH_LOG("    Roughness map path = {0}", texturePath);
					auto texture = Texture2D::Create(texturePath);
					if (texture->Loaded())
					{
						mi->Set("uRoughnessTexture", texture);
						mi->Set("uMaterialUniforms.Roughness", 1.0f);
					}
					else
					{
						NR_CORE_ERROR("    Could not load roughness map}");
						fallback = true;
					}
				}

				if (fallback)
				{
					NR_MESH_LOG("    No roughness map");
					mi->Set("uRoughnessTexture", whiteTexture);
					mi->Set("uMaterialUniforms.Roughness", roughness);
				}

				// Metalness map
				if (aiMaterial->Get("$raw.ReflectionFactor|file", aiPTI_String, 0, aiTexPath) == AI_SUCCESS)
				{
					std::filesystem::path path = filename;
					auto parentPath = path.parent_path();
					parentPath /= std::string(aiTexPath.data);
					std::string texturePath = parentPath.string();

					auto texture = Texture2D::Create(texturePath);
					if (texture->Loaded())
					{
						NR_MESH_LOG("  Metalness map path = {0}", texturePath);
						mi->Set("uMetalnessTexture", texture);
						mi->Set("uMetalnessTexToggle", 1.0f);
					}
					else
					{
						NR_CORE_ERROR("Could not load texture: {0}", texturePath);
					}
				}
				else
				{
					NR_MESH_LOG("    No metalness texture");
					//TODO: mi->Set("uMetalness", metalness);
				}

				bool metalnessTextureFound = false;
				for (uint32_t p = 0; p < aiMaterial->mNumProperties; ++p)
				{
					auto prop = aiMaterial->mProperties[p];

					if (prop->mType == aiPTI_String)
					{
						uint32_t strLength = *(uint32_t*)prop->mData;
						std::string str(prop->mData + 4, strLength);

						std::string key = prop->mKey.data;
						if (key == "$raw.ReflectionFactor|file")
						{
							std::filesystem::path path = modelsDirectory;
							auto parentPath = path.parent_path();
							parentPath /= str;
							std::string texturePath = parentPath.string();

							NR_MESH_LOG("    Metalness map path = {0}", texturePath);
							auto texture = Texture2D::Create(texturePath);
							if (texture->Loaded())
							{
								metalnessTextureFound = true;
								mi->Set("uMetalnessTexture", texture);
								mi->Set("uMaterialUniforms.Metalness", 1.0f);
							}
							else
							{
								NR_CORE_ERROR("    Could not load Metalness texture");
							}
							break;
						}
					}
				}

				fallback = !metalnessTextureFound;
				if (fallback)
				{
					NR_MESH_LOG("    No metalness map");
					mi->Set("uMetalnessTexture", whiteTexture);
					mi->Set("uMaterialUniforms.Metalness", metalness);
				}
			}

			NR_MESH_LOG("------------------------");
		}
		else
		{
			auto mi = Material::Create(Renderer::GetShaderLibrary()->Get("PBR_Static"), "NotRed-Default");
			mi->Set("uMaterialUniforms.AlbedoColor", glm::vec3(0.8f));
			mi->Set("uMaterialUniforms.Emission", 0.0f);
			mi->Set("uMaterialUniforms.Metalness", 0.0f);
			mi->Set("uMaterialUniforms.Roughness", 0.8f);
			mi->Set("uMaterialUniforms.UseNormalMap", false);

			mi->Set("uAlbedoTexture", whiteTexture);
			mi->Set("uMetalnessTexture", whiteTexture);
			mi->Set("uRoughnessTexture", whiteTexture);

			mMaterials.push_back(mi);
		}

		if (mIsAnimated)
		{
			mVertexBuffer = VertexBuffer::Create(mAnimatedVertices.data(), (uint32_t)(mAnimatedVertices.size() * sizeof(AnimatedVertex)));
		}
		else
		{
			mVertexBuffer = VertexBuffer::Create(mStaticVertices.data(), (uint32_t)(mStaticVertices.size() * sizeof(Vertex)));
		}

		mIndexBuffer = IndexBuffer::Create(mIndices.data(), (uint32_t)(mIndices.size() * sizeof(Index)));
	}

	MeshAsset::MeshAsset(const std::vector<Vertex>& vertices, const std::vector<Index>& indices, const glm::mat4& transform)
		: mStaticVertices(vertices), mIndices(indices), mIsAnimated(false)
	{
		Submesh submesh;
		submesh.BaseVertex = 0;
		submesh.BaseIndex = 0;
		submesh.IndexCount = (uint32_t)indices.size() * 3u;
		submesh.Transform = transform;
		mSubmeshes.push_back(submesh);

		mVertexBuffer = VertexBuffer::Create(mStaticVertices.data(), (uint32_t)(mStaticVertices.size() * sizeof(Vertex)));
		mIndexBuffer = IndexBuffer::Create(mIndices.data(), (uint32_t)(mIndices.size() * sizeof(Index)));
	}

	MeshAsset::MeshAsset(int particleCount)
		: mIsAnimated(false)
	{
		{
			for (uint32_t i = 0; i < particleCount; ++i)
			{
				ParticleVertex leftBot;
				leftBot.Position = { -0.5f, 0.0f, -0.5f };
				leftBot.Index = (float)i;
				mParticleVertices.push_back(leftBot);

				ParticleVertex rightBot;
				rightBot.Position = { 0.5f, 0.0f, -0.5f };
				rightBot.Index = (float)i;
				mParticleVertices.push_back(rightBot);

				ParticleVertex leftTop;
				leftTop.Position = { -0.5f, 0.0f,  0.5f };
				leftTop.Index = (float)i;
				mParticleVertices.push_back(leftTop);

				ParticleVertex rightTop;
				rightTop.Position = { 0.5f, 0.0f,  0.5f };
				rightTop.Index = (float)i;
				mParticleVertices.push_back(rightTop);

				unsigned int index = i * 4;
				Index indexA = { index + 0, index + 1, index + 2 };
				Index indexB = { index + 1, index + 3, index + 2 };
				mIndices.push_back(indexA);
				mIndices.push_back(indexB);
			}
		}

		auto mi = Material::Create(Renderer::GetShaderLibrary()->Get("Particle"), "Particle-Effect");
		mi->Set("uGalaxySpecs.StarColor", glm::vec3(1.0f, 1.0f, 1.0f));
		mi->Set("uGalaxySpecs.DustColor", glm::vec3(0.388f, 0.333f, 1.0f));
		mi->Set("uGalaxySpecs.h2RegionColor", glm::vec3(0.8f, 0.071f, 0.165f));
		mMaterials.push_back(mi);

		Submesh& submesh = mSubmeshes.emplace_back();
		submesh.BaseVertex = 0;
		submesh.BaseIndex = 0;
		submesh.IndexCount = mIndices.size() * 3;
		submesh.VertexCount = mParticleVertices.size();
		submesh.Transform = glm::mat4(1.0f);
		submesh.MaterialIndex = 0;
		submesh.MeshName = "Particles";

		mVertexBuffer = VertexBuffer::Create(mParticleVertices.data(), mParticleVertices.size() * sizeof(ParticleVertex));
		mIndexBuffer = IndexBuffer::Create(mIndices.data(), mIndices.size() * sizeof(Index));
	}

	const aiNodeAnim* FindNodeAnim(const aiAnimation* animation, const std::string& nodeName)
	{
		for (uint32_t i = 0; i < animation->mNumChannels; ++i)
		{
			const aiNodeAnim* nodeAnim = animation->mChannels[i];
			if (std::string(nodeAnim->mNodeName.data) == nodeName)
			{
				return nodeAnim;
			}
		}
		return nullptr;
	}

	uint32_t FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
	{
		for (uint32_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; ++i)
		{
			if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime)
			{
				return i;
			}
		}

		return 0;
	}

	uint32_t FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
	{
		NR_CORE_ASSERT(pNodeAnim->mNumRotationKeys > 0);

		for (uint32_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; ++i)
		{
			if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime)
			{
				return i;
			}
		}

		return 0;
	}

	uint32_t FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim)
	{
		NR_CORE_ASSERT(pNodeAnim->mNumScalingKeys > 0);

		for (uint32_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; ++i)
		{
			if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime)
			{
				return i;
			}
		}

		return 0;
	}

	void MeshAsset::TraverseNodes(aiNode* node, const glm::mat4& parentTransform, uint32_t level)
	{
		glm::mat4 transform = parentTransform * AssimpMat4ToMat4(node->mTransformation);
		mNodeMap[node].resize(node->mNumMeshes);
		for (uint32_t i = 0; i < node->mNumMeshes; ++i)
		{
			uint32_t mesh = node->mMeshes[i];
			auto& submesh = mSubmeshes[mesh];
			submesh.NodeName = node->mName.C_Str();
			submesh.Transform = transform;
			mNodeMap[node][i] = mesh;
		}

		for (uint32_t i = 0; i < node->mNumChildren; ++i)
		{
			TraverseNodes(node->mChildren[i], transform, level + 1);
		}
	}

	glm::vec3 InterpolateTranslation(float animationTime, const aiNodeAnim* nodeAnim)
	{
		if (nodeAnim->mNumPositionKeys == 1)
		{
			auto v = nodeAnim->mPositionKeys[0].mValue;
			return { v.x, v.y, v.z };
		}

		uint32_t positionIndex = FindPosition(animationTime, nodeAnim);
		uint32_t nextPositionIndex = (positionIndex + 1);
		NR_CORE_ASSERT(nextPositionIndex < nodeAnim->mNumPositionKeys);

		float deltaTime = (float)(nodeAnim->mPositionKeys[nextPositionIndex].mTime - nodeAnim->mPositionKeys[positionIndex].mTime);
		float factor = (animationTime - (float)nodeAnim->mPositionKeys[positionIndex].mTime) / deltaTime;
		NR_CORE_ASSERT(factor <= 1.0f, "Factor must be below 1.0f");
		factor = glm::clamp(factor, 0.0f, 1.0f);

		const aiVector3D& start = nodeAnim->mPositionKeys[positionIndex].mValue;
		const aiVector3D& end = nodeAnim->mPositionKeys[nextPositionIndex].mValue;
		aiVector3D delta = end - start;
		auto aiVec = start + factor * delta;
		return { aiVec.x, aiVec.y, aiVec.z };
	}


	glm::quat InterpolateRotation(float animationTime, const aiNodeAnim* nodeAnim)
	{
		if (nodeAnim->mNumRotationKeys == 1)
		{
			auto v = nodeAnim->mRotationKeys[0].mValue;
			return glm::quat(v.w, v.x, v.y, v.z);
		}

		uint32_t rotationIndex = FindRotation(animationTime, nodeAnim);
		uint32_t nextRotationIndex = (rotationIndex + 1);
		NR_CORE_ASSERT(nextRotationIndex < nodeAnim->mNumRotationKeys);

		float DeltaTime = (float)(nodeAnim->mRotationKeys[nextRotationIndex].mTime - nodeAnim->mRotationKeys[rotationIndex].mTime);
		float factor = (animationTime - (float)nodeAnim->mRotationKeys[rotationIndex].mTime) / DeltaTime;
		NR_CORE_ASSERT(factor <= 1.0f, "Factor must be below 1.0f");
		factor = glm::clamp(factor, 0.0f, 1.0f);

		const aiQuaternion& startRotationQ = nodeAnim->mRotationKeys[rotationIndex].mValue;
		const aiQuaternion& endRotationQ = nodeAnim->mRotationKeys[nextRotationIndex].mValue;
		auto q = aiQuaternion();
		aiQuaternion::Interpolate(q, startRotationQ, endRotationQ, factor);
		q = q.Normalize();
		return glm::quat(q.w, q.x, q.y, q.z);
	}

	glm::vec3 InterpolateScale(float animationTime, const aiNodeAnim* nodeAnim)
	{
		if (nodeAnim->mNumScalingKeys == 1)
		{
			auto v = nodeAnim->mScalingKeys[0].mValue;
			return { v.x, v.y, v.z };
		}

		uint32_t index = FindScaling(animationTime, nodeAnim);
		uint32_t nextIndex = (index + 1);
		NR_CORE_ASSERT(nextIndex < nodeAnim->mNumScalingKeys);

		float deltaTime = (float)(nodeAnim->mScalingKeys[nextIndex].mTime - nodeAnim->mScalingKeys[index].mTime);
		float factor = (animationTime - (float)nodeAnim->mScalingKeys[index].mTime) / deltaTime;
		NR_CORE_ASSERT(factor <= 1.0f, "Factor must be below 1.0f");
		factor = glm::clamp(factor, 0.0f, 1.0f);

		const auto& start = nodeAnim->mScalingKeys[index].mValue;
		const auto& end = nodeAnim->mScalingKeys[nextIndex].mValue;
		auto delta = end - start;
		auto aiVec = start + factor * delta;
		return { aiVec.x, aiVec.y, aiVec.z };
	}

	void MeshAsset::ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const glm::mat4& parentTransform)
	{
		std::string name(pNode->mName.data);
		const aiAnimation* animation = mScene->mAnimations[0];
		glm::mat4 nodeTransform(AssimpMat4ToMat4(pNode->mTransformation));
		const aiNodeAnim* nodeAnim = FindNodeAnim(animation, name);

		if (nodeAnim)
		{
			glm::vec3 translation = InterpolateTranslation(AnimationTime, nodeAnim);
			glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(translation.x, translation.y, translation.z));

			glm::quat rotation = InterpolateRotation(AnimationTime, nodeAnim);
			glm::mat4 rotationMatrix = glm::toMat4(rotation);

			glm::vec3 scale = InterpolateScale(AnimationTime, nodeAnim);
			glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scale.x, scale.y, scale.z));

			nodeTransform = translationMatrix * rotationMatrix * scaleMatrix;
		}

		glm::mat4 transform = parentTransform * nodeTransform;

		if (mBoneMapping.find(name) != mBoneMapping.end())
		{
			uint32_t BoneIndex = mBoneMapping[name];
			mBoneInfo[BoneIndex].FinalTransformation = mInverseTransform * transform * mBoneInfo[BoneIndex].BoneOffset;
		}

		for (uint32_t i = 0; i < pNode->mNumChildren; ++i)
		{
			ReadNodeHierarchy(AnimationTime, pNode->mChildren[i], transform);
		}
	}

	void MeshAsset::DumpVertexBuffer()
	{
		NR_MESH_LOG("------------------------------------------------------");
		NR_MESH_LOG("Vertex Buffer Dump");
		NR_MESH_LOG("Mesh: {0}", mFilePath);
		if (mIsAnimated)
		{
			for (size_t i = 0; i < mAnimatedVertices.size(); i++)
			{
				auto& vertex = mAnimatedVertices[i];
				NR_MESH_LOG("Vertex: {0}", i);
				NR_MESH_LOG("Position: {0}, {1}, {2}", vertex.Position.x, vertex.Position.y, vertex.Position.z);
				NR_MESH_LOG("Normal: {0}, {1}, {2}", vertex.Normal.x, vertex.Normal.y, vertex.Normal.z);
				NR_MESH_LOG("Binormal: {0}, {1}, {2}", vertex.Binormal.x, vertex.Binormal.y, vertex.Binormal.z);
				NR_MESH_LOG("Tangent: {0}, {1}, {2}", vertex.Tangent.x, vertex.Tangent.y, vertex.Tangent.z);
				NR_MESH_LOG("TexCoord: {0}, {1}", vertex.Texcoord.x, vertex.Texcoord.y);
				NR_MESH_LOG("--");
			}
		}
		else
		{
			for (size_t i = 0; i < mStaticVertices.size(); i++)
			{
				auto& vertex = mStaticVertices[i];
				NR_MESH_LOG("Vertex: {0}", i);
				NR_MESH_LOG("Position: {0}, {1}, {2}", vertex.Position.x, vertex.Position.y, vertex.Position.z);
				NR_MESH_LOG("Normal: {0}, {1}, {2}", vertex.Normal.x, vertex.Normal.y, vertex.Normal.z);
				NR_MESH_LOG("Binormal: {0}, {1}, {2}", vertex.Binormal.x, vertex.Binormal.y, vertex.Binormal.z);
				NR_MESH_LOG("Tangent: {0}, {1}, {2}", vertex.Tangent.x, vertex.Tangent.y, vertex.Tangent.z);
				NR_MESH_LOG("TexCoord: {0}, {1}", vertex.Texcoord.x, vertex.Texcoord.y);
				NR_MESH_LOG("--");
			}
		}
		NR_MESH_LOG("------------------------------------------------------");
	}


	Mesh::Mesh(Ref<MeshAsset> meshAsset)
		: mMeshAsset(meshAsset)
	{
		SetSubmeshes({});

		const auto& meshMaterials = meshAsset->GetMaterials();
		mMaterials = Ref<MaterialTable>::Create(static_cast<uint32_t>(meshMaterials.size()));
		for (size_t i = 0; i < meshMaterials.size(); ++i)
		{
			mMaterials->SetMaterial(static_cast<uint32_t>(i), Ref<MaterialAsset>::Create(meshMaterials[i]));
		}

		if (mMeshAsset->IsAnimated())
		{
			mBoneTransformUBs.resize(Renderer::GetConfig().FramesInFlight);
			for (auto& ub : mBoneTransformUBs) {
				ub = UniformBuffer::Create(meshAsset->mBoneCount * sizeof(glm::mat4), 0);
			}
		}
	}

	Mesh::Mesh(Ref<MeshAsset> meshAsset, const std::vector<uint32_t>& submeshes)
		: mMeshAsset(meshAsset)
	{
		SetSubmeshes(submeshes);

		const auto& meshMaterials = meshAsset->GetMaterials();
		mMaterials = Ref<MaterialTable>::Create(static_cast<uint32_t>(meshMaterials.size()));
		for (size_t i = 0; i < meshMaterials.size(); ++i)
		{
			mMaterials->SetMaterial(static_cast<uint32_t>(i), Ref<MaterialAsset>::Create(meshMaterials[static_cast<uint32_t>(i)]));
		}

		if (mMeshAsset->IsAnimated())
		{
			mBoneTransformUBs.resize(Renderer::GetConfig().FramesInFlight);
			for (auto& ub : mBoneTransformUBs) 
			{
				ub = UniformBuffer::Create(meshAsset->mBoneCount * sizeof(glm::mat4), 0);
			}
		}
	}

	Mesh::Mesh(const Ref<Mesh>& other)
		: mMeshAsset(other->mMeshAsset), mMaterials(other->mMaterials)
	{
		SetSubmeshes(other->mSubmeshes);

		if (mMeshAsset->IsAnimated())
		{
			mBoneTransformUBs.resize(Renderer::GetConfig().FramesInFlight);
			for (auto& ub : mBoneTransformUBs) 
			{
				ub = UniformBuffer::Create(mMeshAsset->mBoneCount * sizeof(glm::mat4), 0);
			}
		}
	}

	void Mesh::Update(float dt)
	{
		if (IsAnimated())
		{
			if (mAnimationPlaying)
			{
				mWorldTime += dt;

				float ticksPerSecond = (float)(mMeshAsset->mScene->mAnimations[0]->mTicksPerSecond != 0 ? mMeshAsset->mScene->mAnimations[0]->mTicksPerSecond : 25.0f) * mTimeMultiplier;
				mAnimationTime += dt * ticksPerSecond;
				mAnimationTime = fmod(mAnimationTime, (float)mMeshAsset->mScene->mAnimations[0]->mDuration);
			}

			BoneTransform(mAnimationTime);
		}
	}

	void Mesh::SetSubmeshes(const std::vector<uint32_t>& submeshes)
	{
		if (!submeshes.empty())
		{
			mSubmeshes = submeshes;
		}
		else
		{
			const auto& submeshes = mMeshAsset->GetSubmeshes();
			mSubmeshes.resize(submeshes.size());
			for (uint32_t i = 0; i < submeshes.size(); ++i)
			{
				mSubmeshes[i] = i;
			}
		}
	}

	void Mesh::BoneTransform(float time)
	{
		NR_PROFILE_FUNC();
		{
			NR_PROFILE_SCOPE_DYNAMIC("Sample animation");
			mMeshAsset->ReadNodeHierarchy(time, mMeshAsset->mScene->mRootNode, glm::mat4(1.0f));
			mBoneTransforms.resize(mMeshAsset->mBoneCount);
			for (size_t i = 0; i < mMeshAsset->mBoneCount; ++i)
			{
				mBoneTransforms[i] = mMeshAsset->mBoneInfo[i].FinalTransformation;
			}
		}

		{
			NR_PROFILE_SCOPE_DYNAMIC("Upload bone transform UB");
			// upload to GPU
			Ref<Mesh> instance = this;
			Renderer::Submit([instance]() mutable {
				instance->mBoneTransformUBs[Renderer::GetCurrentFrameIndex()]->RT_SetData(instance->mBoneTransforms.data(), static_cast<uint32_t>(instance->mBoneTransforms.size() * sizeof(glm::mat4)));
				});
		}
	}
}