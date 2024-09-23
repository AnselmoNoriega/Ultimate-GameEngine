#include "nrpch.h"
#include "CookingFactory.h"

#include "PhysicsInternal.h"

namespace NR
{
	struct CookingData
	{
		physx::PxCookingParams CookingParameters;

		CookingData(const physx::PxTolerancesScale& scale)
			: CookingParameters(scale)
		{
		}
	};

	static CookingData* sCookingData = nullptr;
	void CookingFactory::Initialize()
	{
		sCookingData = new CookingData(PhysicsInternal::GetPhysicsSDK().getTolerancesScale());
		sCookingData->CookingParameters.meshWeldTolerance = 0.1f;
		sCookingData->CookingParameters.meshPreprocessParams = physx::PxMeshPreprocessingFlag::eWELD_VERTICES;
		sCookingData->CookingParameters.midphaseDesc = physx::PxMeshMidPhase::eBVH34;
	}

	void CookingFactory::Shutdown()
	{
		delete sCookingData;
	}

	CookingResult CookingFactory::CookConvexMesh(const Ref<Mesh>& mesh, std::vector<MeshColliderData>& outData)
	{
		const auto& vertices = mesh->GetVertices();
		const auto& indices = mesh->GetIndices();

		for (const auto& submesh : mesh->GetSubmeshes())
		{
			physx::PxConvexMeshDesc convexDesc;
			convexDesc.points.count = submesh.VertexCount;
			convexDesc.points.stride = sizeof(Vertex);
			convexDesc.points.data = &vertices[submesh.BaseVertex];
			convexDesc.indices.count = submesh.IndexCount / 3;
			convexDesc.indices.data = &indices[submesh.BaseIndex / 3];
			convexDesc.indices.stride = sizeof(Index);
			convexDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX | physx::PxConvexFlag::eSHIFT_VERTICES;

			physx::PxDefaultMemoryOutputStream buf;
			physx::PxConvexMeshCookingResult::Enum result;
			if (!PxCookConvexMesh(sCookingData->CookingParameters, convexDesc, buf, &result))
			{
				NR_CORE_ERROR("Failed to cook convex mesh {0}", submesh.MeshName);
				outData.clear();
				return PhysicsUtils::FromPhysicsCookingResult(result);
			}

			MeshColliderData& data = outData.emplace_back();
			data.Size = buf.getSize();
			data.Data = new byte[data.Size];
			data.Transform = submesh.Transform;
			memcpy(data.Data, buf.getData(), data.Size);
		}

		return CookingResult::Success;
	}

	CookingResult CookingFactory::CookTriangleMesh(const Ref<Mesh>& mesh, std::vector<MeshColliderData>& outData)
	{
		const auto& vertices = mesh->GetVertices();
		const auto& indices = mesh->GetIndices();
		for (const auto& submesh : mesh->GetSubmeshes())
		{
			physx::PxTriangleMeshDesc triangleDesc;
			triangleDesc.points.count = submesh.VertexCount;
			triangleDesc.points.stride = sizeof(Vertex);
			triangleDesc.points.data = &vertices[submesh.BaseVertex];
			triangleDesc.triangles.count = submesh.IndexCount / 3;
			triangleDesc.triangles.data = &indices[submesh.BaseIndex / 3];
			triangleDesc.triangles.stride = sizeof(Index);
			physx::PxDefaultMemoryOutputStream buf;
			physx::PxTriangleMeshCookingResult::Enum result;
			if (!PxCookTriangleMesh(sCookingData->CookingParameters, triangleDesc, buf, &result))
			{
				NR_CORE_ERROR("Failed to cook convex mesh {0}", submesh.MeshName);
				outData.clear();
				return PhysicsUtils::FromPhysicsCookingResult(result);
			}
			MeshColliderData& data = outData.emplace_back();
			data.Size = buf.getSize();
			data.Data = new byte[data.Size];
			data.Transform = submesh.Transform;
			memcpy(data.Data, buf.getData(), data.Size);
		}
	}

}