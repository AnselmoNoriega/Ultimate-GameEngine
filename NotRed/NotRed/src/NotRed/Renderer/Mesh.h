#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "NotRed/Renderer/Pipeline.h"
#include "NotRed/Renderer/VertexBuffer.h"
#include "NotRed/Renderer/IndexBuffer.h"
#include "NotRed/Renderer/Shader.h"
#include "NotRed/Renderer/Material.h"

#include "NotRed/Util/Asset.h"

#include "NotRed/Math/AABB.h"

struct aiNode;
struct aiAnimation;
struct aiNodeAnim;
struct aiScene;

namespace Assimp 
{
	class Importer;
}

namespace NR
{
	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec3 Tangent;
		glm::vec3 Binormal;
		glm::vec2 Texcoord;
	};

	struct AnimatedVertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec3 Tangent;
		glm::vec3 Binormal;
		glm::vec2 Texcoord;

		uint32_t IDs[4] = { 0, 0,0, 0 };
		float Weights[4]{ 0.0f, 0.0f, 0.0f, 0.0f };

		void AddBoneData(uint32_t BoneID, float Weight)
		{
			for (size_t i = 0; i < 4; ++i)
			{
				if (Weights[i] == 0.0)
				{
					IDs[i] = BoneID;
					Weights[i] = Weight;
					return;
				}
			}

			NR_CORE_WARN("Vertex has more than four bones/weights affecting it, extra data will be discarded (BoneID={0}, Weight={1})", BoneID, Weight);
		}
	};

	static const int NumAttributes = 5;

	struct Index
	{
		uint32_t V1, V2, V3;
	};

	struct BoneInfo
	{
		glm::mat4 BoneOffset;
		glm::mat4 FinalTransformation;
	};

	struct VertexBoneData
	{
		uint32_t IDs[4];
		float Weights[4];

		VertexBoneData()
		{
			memset(IDs, 0, sizeof(IDs));
			memset(Weights, 0, sizeof(Weights));
		};

		void AddBoneData(uint32_t BoneID, float Weight)
		{
			for (size_t i = 0; i < 4; ++i)
			{
				if (Weights[i] == 0.0)
				{
					IDs[i] = BoneID;
					Weights[i] = Weight;
					return;
				}
			}

			NR_CORE_ASSERT(false, "Too many weights for a bone!");
		}
	};

	struct Triangle
	{
		Vertex V0, V1, V2;

		Triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2)
			: V0(v0), V1(v1), V2(v2) {}
	};

	class Submesh
	{
	public:
		uint32_t BaseVertex;
		uint32_t BaseIndex;
		uint32_t IndexCount;
		uint32_t VertexCount;
		uint32_t MaterialIndex;

		glm::mat4 Transform;
		glm::mat4 LocalTransform;
		AABB BoundingBox;

		std::string NodeName, MeshName;
	};

	class Mesh : public Asset
	{
	public:
		Mesh(const std::string& filename);
		Mesh(const std::vector<Vertex>& vertices, const std::vector<Index>& indices, const glm::mat4& transform);
		~Mesh();

		void Update(float dt);
		void DumpVertexBuffer();

		const std::string& GetFilePath() const { return mFilePath; }

		bool IsAnimated() const { return mIsAnimated; }

		std::vector<Submesh>& GetSubmeshes() { return mSubmeshes; }
		const std::vector<Submesh>& GetSubmeshes() const { return mSubmeshes; }

		const std::vector<Vertex>& GetStaticVertices() const { return mStaticVertices; }
		const std::vector<Index>& GetIndices() const { return mIndices; }

		Ref<Shader> GetMeshShader() { return mMeshShader; }
		Ref<Material> GetMaterial() { return mBaseMaterial; }
		std::vector<Ref<MaterialInstance>>& GetMaterials() { return mMaterials; }
		const std::vector<Ref<Texture2D>>& GetTextures() const { return mTextures; }

		const std::vector<Triangle> GetTriangleCache(uint32_t index) const { return mTriangleCache.at(index); }

	private:
		void BoneTransform(float time);
		void ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const glm::mat4& ParentTransform);
		void TraverseNodes(aiNode* node, const glm::mat4& parentTransform = glm::mat4(1.0f), uint32_t level = 0);

		const aiNodeAnim* FindNodeAnim(const aiAnimation* animation, const std::string& nodeName);
		uint32_t FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
		uint32_t FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
		uint32_t FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
		glm::vec3 InterpolateTranslation(float animationTime, const aiNodeAnim* nodeAnim);
		glm::quat InterpolateRotation(float animationTime, const aiNodeAnim* nodeAnim);
		glm::vec3 InterpolateScale(float animationTime, const aiNodeAnim* nodeAnim);

	private:
		std::vector<Submesh> mSubmeshes;

		std::unique_ptr<Assimp::Importer> mImporter;

		glm::mat4 mInverseTransform;

		uint32_t mBoneCount = 0;
		std::vector<BoneInfo> mBoneInfo;

		Ref<Pipeline> mPipeline;
		Ref<VertexBuffer> mVertexBuffer;
		Ref<IndexBuffer> mIndexBuffer;

		std::vector<Vertex> mStaticVertices;
		std::vector<AnimatedVertex> mAnimatedVertices;
		std::vector<Index> mIndices;
		std::unordered_map<std::string, uint32_t> mBoneMapping;
		std::vector<glm::mat4> mBoneTransforms;
		const aiScene* mScene;

		// Materials
		Ref<Shader> mMeshShader;
		Ref<Material> mBaseMaterial;
		std::vector<Ref<Texture2D>> mTextures;
		std::vector<Ref<Texture2D>> mNormalMaps;
		std::vector<Ref<MaterialInstance>> mMaterials;

		std::unordered_map<uint32_t, std::vector<Triangle>> mTriangleCache;

		// Animation
		bool mIsAnimated = false;
		float mAnimationTime = 0.0f;
		float mWorldTime = 0.0f;
		float mTimeMultiplier = 1.0f;
		bool mAnimationPlaying = true;

		std::string mFilePath;

		friend class Renderer;
		friend class SceneHierarchyPanel;
	};
}