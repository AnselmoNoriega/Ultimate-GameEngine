#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "NotRed/Asset/Asset.h"

#include "NotRed/Renderer/Pipeline.h"
#include "NotRed/Renderer/VertexBuffer.h"
#include "NotRed/Renderer/IndexBuffer.h"
#include "NotRed/Renderer/Shader.h"
#include "NotRed/Renderer/Material.h"

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
	struct ParticleVertex
	{
		glm::vec3 Position;
	};

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

		glm::mat4 Transform{ 1.0f };
		AABB BoundingBox;

		std::string NodeName, MeshName;
	};

	class MeshAsset : public Asset
	{
	public:
		MeshAsset(const std::string& filename);
		MeshAsset(const std::vector<Vertex>& vertices, const std::vector<Index>& indices, const glm::mat4& transform);
		MeshAsset(int particleCount);
		virtual ~MeshAsset() = default;

		void DumpVertexBuffer();

		std::vector<Submesh>& GetSubmeshes() { return mSubmeshes; }
		const std::vector<Submesh>& GetSubmeshes() const { return mSubmeshes; }

		const std::vector<Vertex>& GetVertices() const { return mStaticVertices; }
		const std::vector<Index>& GetIndices() const { return mIndices; }

		Ref<Shader> GetMeshShader() { return mMeshShader; }
		std::vector<Ref<Material>>& GetMaterials() { return mMaterials; }
		const std::vector<Ref<Material>>& GetMaterials() const { return mMaterials; }
		const std::vector<Ref<Texture2D>>& GetTextures() const { return mTextures; }
		const std::string& GetFilePath() const { return mFilePath; }

		const std::vector<Triangle> GetTriangleCache(uint32_t index) const { return mTriangleCache.at(index); }

		Ref<VertexBuffer> GetVertexBuffer() { return mVertexBuffer; }
		Ref<IndexBuffer> GetIndexBuffer() { return mIndexBuffer; }
		const VertexBufferLayout& GetVertexBufferLayout() const { return mVertexBufferLayout; }

		static AssetType GetStaticType() { return AssetType::MeshAsset; }
		virtual AssetType GetAssetType() const override { return AssetType::MeshAsset; }

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

		Ref<VertexBuffer> mVertexBuffer;
		Ref<IndexBuffer> mIndexBuffer;
		VertexBufferLayout mVertexBufferLayout;

		std::vector<Vertex> mStaticVertices;
		std::vector<AnimatedVertex> mAnimatedVertices;
		std::vector<ParticleVertex> mParticleVertices;

		std::vector<Index> mIndices;
		std::unordered_map<std::string, uint32_t> mBoneMapping;
		std::vector<glm::mat4> mBoneTransforms;
		const aiScene* mScene;

		// Materials
		Ref<Shader> mMeshShader;
		std::vector<Ref<Texture2D>> mTextures;
		std::vector<Ref<Texture2D>> mNormalMaps;
		std::vector<Ref<Material>> mMaterials;

		std::unordered_map<uint32_t, std::vector<Triangle>> mTriangleCache;

		// Animation
		bool mIsAnimated = false;
		float mTimeMultiplier = 1.0f;
		bool mAnimationPlaying = true;

		std::string mFilePath;

	private:
		friend class Renderer;
		friend class VKRenderer;
		friend class GLRenderer;
		friend class SceneHierarchyPanel;
		friend class MeshViewerPanel;
	};

	class Mesh : public Asset
	{
	public:
		explicit Mesh(Ref<MeshAsset> meshAsset);
		Mesh(Ref<MeshAsset> meshAsset, const std::vector<uint32_t>& submeshes);
		virtual ~Mesh() = default;

		void Update(float dt);

		const std::vector<uint32_t>& GetSubmeshes() const { return mSubmeshes; }
		void SetSubmeshes(const std::vector<uint32_t>& submeshes) { mSubmeshes = submeshes; }

		Ref<MeshAsset> GetMeshAsset() { return mMeshAsset; }
		void SetMeshAsset(Ref<MeshAsset> meshAsset) { mMeshAsset = meshAsset; }

		Ref<Shader> GetMeshShader() { return mMeshShader; }
		std::vector<Ref<Material>>& GetMaterials() { return mMaterials; }
		const std::vector<Ref<Material>>& GetMaterials() const { return mMaterials; }

		static AssetType GetStaticType() { return AssetType::Mesh; }
		virtual AssetType GetAssetType() const override { return AssetType::Mesh; }

	private:
		Ref<MeshAsset> mMeshAsset;
		std::vector<uint32_t> mSubmeshes;

		uint32_t mBoneCount = 0;
		std::vector<BoneInfo> mBoneInfo;

		// Materials
		Ref<Shader> mMeshShader;
		std::vector<Ref<Material>> mMaterials;

		// Animation
		bool mIsAnimated = false;
		float mAnimationTime = 0.0f;
		float mWorldTime = 0.0f;
		float mTimeMultiplier = 1.0f;
		bool mAnimationPlaying = true;

	private:
		friend class Renderer;
		friend class VKRenderer;
		friend class GLRenderer;
		friend class SceneHierarchyPanel;
		friend class MeshViewerPanel;
	};
}