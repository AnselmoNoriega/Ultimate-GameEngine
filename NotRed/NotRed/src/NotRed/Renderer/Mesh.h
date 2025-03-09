#pragma once

#include <vector>
#include <glm/glm.hpp>

#include <ozz/base/maths/simd_math.h>

#include "NotRed/Asset/Asset.h"

#include "NotRed/Math/AABB.h"

#include "NotRed/Renderer/Animation.h"
#include "NotRed/Renderer/IndexBuffer.h"
#include "NotRed/Renderer/MaterialAsset.h"
#include "NotRed/Renderer/Pipeline.h"
#include "NotRed/Renderer/Shader.h"
#include "NotRed/Renderer/UniformBuffer.h"
#include "NotRed/Renderer/VertexBuffer.h"

struct aiNode;
struct aiAnimation;
struct aiNodeAnim;
struct aiScene;

namespace Assimp {
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

	struct BoneInfluence
	{
		uint32_t IDs[4] = { 0, 0, 0, 0 };
		float Weights[4]{ 0.0f, 0.0f, 0.0f, 0.0f };

		void AddBoneData(uint32_t BoneID, float Weight)
		{
			if (Weight < 0.0f || Weight > 1.0f)
			{
				NR_CORE_WARN("Vertex bone weight is out of range. We will clamp it to [0, 1] (BoneID={0}, Weight={1})", BoneID, Weight);
				Weight = std::clamp(Weight, 0.0f, 1.0f);
			}
			if (Weight > 0.0f)
			{
				for (size_t i = 0; i < 4; i++)
				{
					if (Weights[i] == 0.0f)
					{
						IDs[i] = BoneID;
						Weights[i] = Weight;
						return;
					}
				}

				// Note: when importing from assimp we are passing aiProcess_LimitBoneWeights which automatically keeps only the top N (where N defaults to 4)
				//       bone weights (and normalizes the sum to 1), which is exactly what we want.
				//       So, we should never get here.
				NR_CORE_WARN("Vertex has more than four bones/weights affecting it, extra data will be discarded (BoneID={0}, Weight={1})", BoneID, Weight);
			}
		}

		void NormalizeWeights()
		{
			float sumWeights = 0.0f;
			for (size_t i = 0; i < 4; i++)
			{
				sumWeights += Weights[i];
			}
			if (sumWeights > 0.0f)
			{
				for (size_t i = 0; i < 4; i++)
				{
					Weights[i] /= sumWeights;
				}
			}
		}
	};

	static const int NumAttributes = 5;

	struct Index
	{
		uint32_t V1, V2, V3;
	};

	static_assert(sizeof(Index) == 3 * sizeof(uint32_t));

	struct BoneInfo
	{
		ozz::math::Float4x4 SubMeshInverseTransform;
		ozz::math::Float4x4 InverseBindPose;
		uint32_t SubMeshIndex;
		uint32_t JointIndex;

		BoneInfo(ozz::math::Float4x4 subMeshInverseTransform, ozz::math::Float4x4 inverseBindPose, uint32_t subMeshIndex, uint32_t jointIndex)
			: SubMeshInverseTransform(subMeshInverseTransform)
			, InverseBindPose(inverseBindPose)
			, SubMeshIndex(subMeshIndex)
			, JointIndex(jointIndex)
		{}
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
		uint32_t MaterialIndex;
		uint32_t IndexCount;
		uint32_t VertexCount;

		glm::mat4 Transform{ 1.0f }; // World transform
		glm::mat4 LocalTransform{ 1.0f };
		AABB BoundingBox;

		std::string NodeName, MeshName;
	};

	//
	// MeshSource is a representation of an actual asset file on disk
	// Meshes are created from MeshSource
	//
	class MeshSource : public Asset
	{
	public:
		MeshSource(const std::string& filename);
		MeshSource(const std::vector<Vertex>& vertices, const std::vector<Index>& indices, const glm::mat4& transform);
		MeshSource(const std::vector<Vertex>& vertices, const std::vector<Index>& indices, const std::vector<Submesh>& submeshes);
		MeshSource(int squareCount);
		~MeshSource() override;

		void DumpVertexBuffer();

		std::vector<Submesh>& GetSubmeshes() { return mSubmeshes; }
		const std::vector<Submesh>& GetSubmeshes() const { return mSubmeshes; }

		const std::vector<Vertex>& GetVertices() const { return mVertices; }
		const std::vector<Index>& GetIndices() const { return mIndices; }

		bool IsRigged() const { return (bool)mSkeleton; }
		const ozz::animation::Skeleton* GetSkeleton() const { NR_CORE_ASSERT(mSkeleton, "Attempted to access null skeleton!"); return mSkeleton.get(); }
		const std::vector<BoneInfluence>& GetBoneInfluences() const { return mBoneInfluences; }

		std::vector<Ref<Material>>& GetMaterials() { return mMaterials; }
		const std::vector<Ref<Material>>& GetMaterials() const { return mMaterials; }
		const std::string& GetFilePath() const { return mFilePath; }

		const std::vector<Triangle> GetTriangleCache(uint32_t index) const { return mTriangleCache.at(index); }

		Ref<VertexBuffer> GetVertexBuffer() { return mVertexBuffer; }
		Ref<VertexBuffer> GetBoneInfluenceBuffer() { return mBoneInfluenceBuffer; }
		Ref<IndexBuffer> GetIndexBuffer() { return mIndexBuffer; }

		static AssetType GetStaticType() { return AssetType::MeshSource; }
		AssetType GetAssetType() const override { return GetStaticType(); }

		const AABB& GetBoundingBox() const { return mBoundingBox; }

	private:
		void TraverseNodes(aiNode* node, const glm::mat4& parentTransform = glm::mat4(1.0f), uint32_t level = 0);

	private:
		std::vector<Submesh> mSubmeshes;

		std::unique_ptr<Assimp::Importer> mImporter; // note: the importer owns data pointed to by mScene, and mNodeMap and hence must stay in scope for lifetime of MeshSource.

		Ref<VertexBuffer> mVertexBuffer;
		Ref<VertexBuffer> mBoneInfluenceBuffer;
		Ref<IndexBuffer> mIndexBuffer;

		std::vector<Vertex> mVertices;
		std::vector<Index> mIndices;
		std::unordered_map<aiNode*, std::vector<uint32_t>> mNodeMap;
		const aiScene* mScene;

		std::vector<BoneInfluence> mBoneInfluences;
		std::vector<BoneInfo> mBoneInfo;
		ozz::unique_ptr<ozz::animation::Skeleton> mSkeleton;

		std::vector<Ref<Material>> mMaterials;

		std::unordered_map<uint32_t, std::vector<Triangle>> mTriangleCache;

		AABB mBoundingBox;

		std::string mFilePath;

		friend class Scene;
		friend class SceneRenderer;
		friend class Renderer;
		friend class VKRenderer;
		friend class SceneHierarchyPanel;
		friend class MeshViewerPanel;
		friend class Mesh;
	};

	// Dynamic Mesh - supports skeletal animation and retains hierarchy
	class Mesh : public Asset
	{
	public:
		explicit Mesh(Ref<MeshSource> meshSource);
		Mesh(Ref<MeshSource> meshSource, const std::vector<uint32_t>& submeshes);
		Mesh(const Ref<Mesh>& other);
		virtual ~Mesh();

		bool IsRigged() { return mMeshSource && mMeshSource->IsRigged(); }

		std::vector<uint32_t>& GetSubmeshes() { return mSubmeshes; }
		const std::vector<uint32_t>& GetSubmeshes() const { return mSubmeshes; }

		// Pass in an empty vector to set ALL submeshes for MeshSource
		void SetSubmeshes(const std::vector<uint32_t>& submeshes);

		Ref<MeshSource> GetMeshSource() { return mMeshSource; }
		Ref<MeshSource> GetMeshSource() const { return mMeshSource; }
		void SetMeshAsset(Ref<MeshSource> meshSource) { mMeshSource = meshSource; }

		Ref<MaterialTable> GetMaterials() const { return mMaterials; }

		static AssetType GetStaticType() { return AssetType::Mesh; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	private:
		Ref<MeshSource> mMeshSource;
		std::vector<uint32_t> mSubmeshes;

		// Materials
		Ref<MaterialTable> mMaterials;

		friend class Scene;
		friend class Renderer;
		friend class VKRenderer;
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
		~StaticMesh() override;

		std::vector<uint32_t>& GetSubmeshes() { return mSubmeshes; }
		const std::vector<uint32_t>& GetSubmeshes() const { return mSubmeshes; }

		// Pass in an empty vector to set ALL submeshes for MeshSource
		void SetSubmeshes(const std::vector<uint32_t>& submeshes);

		Ref<MeshSource> GetMeshSource() { return mMeshSource; }
		Ref<MeshSource> GetMeshSource() const { return mMeshSource; }
		void SetMeshAsset(Ref<MeshSource> meshAsset) { mMeshSource = meshAsset; }

		Ref<MaterialTable> GetMaterials() const { return mMaterials; }

		static AssetType GetStaticType() { return AssetType::StaticMesh; }
		AssetType GetAssetType() const override { return GetStaticType(); }

	private:
		Ref<MeshSource> mMeshSource;
		std::vector<uint32_t> mSubmeshes;

		// Materials
		Ref<MaterialTable> mMaterials;

		friend class Scene;
		friend class Renderer;
		friend class VKRenderer;
		friend class SceneHierarchyPanel;
		friend class MeshViewerPanel;
	};
}