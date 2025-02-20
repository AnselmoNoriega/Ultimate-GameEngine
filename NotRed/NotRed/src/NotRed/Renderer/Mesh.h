#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "NotRed/Asset/Asset.h"

#include "NotRed/Math/AABB.h"
#include "NotRed/Renderer/Animation.h"

#include "NotRed/Renderer/IndexBuffer.h"
#include "NotRed/Renderer/MaterialAsset.h"
#include "NotRed/Renderer/Pipeline.h"
#include "NotRed/Renderer/Shader.h"
#include "NotRed/Renderer/UniformBuffer.h"
#include "NotRed/Renderer/VertexBuffer.h"

#include <ozz/base/maths/simd_math.h>

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
		float Index;
	};

	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec3 Tangent;
		glm::vec3 Binormal;
		glm::vec2 Texcoord;
	};

	struct SkinnedVertex : public Vertex
	{
		uint32_t IDs[4] = { 0, 0,0, 0 };
		float Weights[4]{ 0.0f, 0.0f, 0.0f, 0.0f };

		SkinnedVertex(const Vertex& vertex) : Vertex(vertex) {}

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
		ozz::math::Float4x4 InverseBindPose;
			
		uint32_t JointIndex;

		BoneInfo(ozz::math::Float4x4 inverseBindPose, uint32_t jointIndex) 
			: InverseBindPose(inverseBindPose), JointIndex(jointIndex) {}
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
		uint32_t BaseVertex{};
		uint32_t BaseIndex{};
		uint32_t IndexCount{};
		uint32_t VertexCount{};
		uint32_t MaterialIndex{};

		glm::mat4 Transform{ 1.0f };
		AABB BoundingBox;

		std::string NodeName, MeshName;
	};

	class MeshSource : public Asset
	{
	public:
		MeshSource(const std::string& filename);
		MeshSource(const std::vector<Vertex>& vertices, const std::vector<Index>& indices, const glm::mat4& transform);
		MeshSource(
			const std::vector<std::vector<Vertex>>& vertices, 
			const std::vector<std::vector<Index>>& indices,
			const std::vector<std::string>& matNames
		);
		MeshSource(int particleCount);
		MeshSource(const std::vector<Vertex>& vertices, const std::vector<Index>& indices, const std::vector<Submesh>& submeshes);
		virtual ~MeshSource() = default;

		void DumpVertexBuffer();

		std::vector<Submesh>& GetSubmeshes() { return mSubmeshes; }
		const std::vector<Submesh>& GetSubmeshes() const { return mSubmeshes; }

		bool IsRigged() const { return (bool)mSkeleton; }
		const std::vector<Vertex>& GetStaticVertices() const { return mStaticVertices; }
		const std::vector<SkinnedVertex>& GetAnimatedVertices() const { return mSkinnedVertices; }
		const std::vector<Index>& GetIndices() const { return mIndices; }

		std::vector<Ref<Material>>& GetMaterials() { return mMaterials; }
		const std::vector<Ref<Material>>& GetMaterials() const { return mMaterials; }
		const std::string& GetFilePath() const { return mFilePath; }

		const std::vector<Triangle> GetTriangleCache(uint32_t index) const { return mTriangleCache.at(index); }

		Ref<VertexBuffer> GetVertexBuffer() { return mVertexBuffer; }
		Ref<IndexBuffer> GetIndexBuffer() { return mIndexBuffer; }

		static AssetType GetStaticType() { return AssetType::MeshSource; }
		virtual AssetType GetAssetType() const override { return AssetType::MeshSource; }

		const AABB& GetBoundingBox() const { return mBoundingBox; }

	private:
		void TraverseNodes(aiNode* node, const glm::mat4& parentTransform = glm::mat4(1.0f), uint32_t level = 0);

	private:
		std::vector<Submesh> mSubmeshes;

		std::unique_ptr<Assimp::Importer> mImporter;

		Ref<VertexBuffer> mVertexBuffer;
		Ref<IndexBuffer> mIndexBuffer;

		std::vector<Vertex> mStaticVertices;
		std::vector<ParticleVertex> mParticleVertices;

		AABB mBoundingBox;

		std::vector<Index> mIndices;
		std::unordered_map<aiNode*, std::vector<uint32_t>> mNodeMap;
		const aiScene* mScene;

		std::vector<SkinnedVertex> mSkinnedVertices;
		std::vector<BoneInfo> mBoneInfo;
		ozz::unique_ptr<ozz::animation::Skeleton> mSkeleton;

		// Materials
		std::vector<Ref<Material>> mMaterials;

		std::unordered_map<uint32_t, std::vector<Triangle>> mTriangleCache;

		std::string mFilePath;

	private:
		friend class Scene;
		friend class Renderer;
		friend class VKRenderer;
		friend class GLRenderer;
		friend class SceneHierarchyPanel;
		friend class MeshViewerPanel;
		friend class Mesh;
	};

	class Mesh : public Asset
	{
	public:
		explicit Mesh(Ref<MeshSource> meshSource);
		Mesh(Ref<MeshSource> meshSource, const std::vector<uint32_t>& submeshes);
		Mesh(const Ref<Mesh>& other);
		virtual ~Mesh() = default;

		void UpdateBoneTransforms(const ozz::vector<ozz::math::Float4x4>& modelSpaceTransforms);

		bool IsRigged() { return mMeshSource && mMeshSource->IsRigged(); }

		Ref<UniformBuffer> GetBoneTransformUB(uint32_t frameIndex) { return mBoneTransformUBs[frameIndex]; }
		std::vector<uint32_t>& GetSubmeshes() { return mSubmeshes; }
		const std::vector<uint32_t>& GetSubmeshes() const { return mSubmeshes; }
		void SetSubmeshes(const std::vector<uint32_t>& submeshes);

		Ref<MeshSource> GetMeshSource() { return mMeshSource; }
		Ref<MeshSource> GetMeshSource() const { return mMeshSource; }
		void SetMeshSource(Ref<MeshSource> meshSource) { mMeshSource = meshSource; }

		Ref<MaterialTable> GetMaterials() const { return mMaterials; }

		static AssetType GetStaticType() { return AssetType::Mesh; }
		virtual AssetType GetAssetType() const override { return AssetType::Mesh; }

		void AddVertices(const std::vector<Vertex>& vertices, uint32_t index);
		void AddIndices(const std::vector<int>& indices, uint32_t index);
		void SetSubmeshesCount(int count);

	private:
		void BoneTransform(float time);

	private:
		Ref<MeshSource> mMeshSource;
		std::vector<uint32_t> mSubmeshes;

		// Materials
		Ref<MaterialTable> mMaterials;

		// Skinning
		ozz::vector<ozz::math::Float4x4> mBoneTransforms;
		std::vector<Ref<UniformBuffer>> mBoneTransformUBs;

	private:
		friend class Scene;
		friend class Renderer;
		friend class VKRenderer;
		friend class GLRenderer;
		friend class SceneHierarchyPanel;
		friend class MeshViewerPanel;
	};

	// Static Mesh - no skeletal animation, flattened hierarchy
	class StaticMesh : public Asset
	{
	public:
		explicit StaticMesh(Ref<MeshSource> meshSource);
		StaticMesh(Ref<MeshSource> meshSource, const std::vector<uint32_t>& submeshes);
		StaticMesh(const Ref<StaticMesh>& other);
		virtual ~StaticMesh();

		std::vector<uint32_t>& GetSubmeshes() { return mSubmeshes; }
		const std::vector<uint32_t>& GetSubmeshes() const { return mSubmeshes; }
		
		void SetSubmeshes(const std::vector<uint32_t>& submeshes);
		
		Ref<MeshSource> GetMeshSource() { return mMeshSource; }
		Ref<MeshSource> GetMeshSource() const { return mMeshSource; }
		
		void SetMeshAsset(Ref<MeshSource> meshAsset) { mMeshSource = meshAsset; }
		
		Ref<MaterialTable> GetMaterials() const { return mMaterials; }
		static AssetType GetStaticType() { return AssetType::StaticMesh; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
	
	private:
		Ref<MeshSource> mMeshSource;
		std::vector<uint32_t> mSubmeshes;
	
		// Materials
		Ref<MaterialTable> mMaterials;

	private:
		friend class Scene;
		friend class Renderer;
		friend class VulkanRenderer;
		friend class OpenGLRenderer;
		friend class SceneHierarchyPanel;
		friend class MeshViewerPanel;
	};
}