#include "nrpch.h"
#include "CookingFactory.h"

#include <filesystem>

#include "PhysicsInternal.h"

#include "NotRed/Asset/AssetManager.h"
#include "NotRed/Project/Project.h"
#include "NotRed/Math/Math.h"
#include "NotRed/Debug/Profiler.h"
#include "NotRed/Core/Timer.h"

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

	namespace Utils 
	{
		static std::filesystem::path GetCacheDirectory()
		{
			return Project::GetCacheDirectory() / "Colliders";
		}

		static void CreateCacheDirectoryIfNeeded()
		{
			std::filesystem::path cacheDirectory = GetCacheDirectory();
			if (!std::filesystem::exists(cacheDirectory))
			{
				std::filesystem::create_directories(cacheDirectory);
			}
		}
	}

	std::vector<MeshColliderData> CookingFactory::mDefaultOutData;
	CookingResult CookingFactory::CookMesh(MeshColliderComponent& component, bool invalidateOld, std::vector<MeshColliderData>& outData)
	{
		NR_SCOPE_TIMER("CookingFactory::CookMesh");

		Utils::CreateCacheDirectoryIfNeeded();
		auto& metadata = AssetManager::GetMetadata(component.CollisionMesh->Handle);
		if (!metadata.IsValid())
		{
			NR_CORE_ERROR("Invalid mesh");
			return CookingResult::Failure;
		}

		CookingResult result = CookingResult::Failure;
		std::filesystem::path filepath = Utils::GetCacheDirectory() / (metadata.FilePath.stem().string() + (component.IsConvex ? "_convex.pxm" : "_tri.pxm"));
		if (invalidateOld)
		{
			component.ProcessedMeshes.clear();
			bool removedCached = std::filesystem::remove(filepath);
			if (!removedCached)
			{
				NR_CORE_WARN("Couldn't delete cached collider data for {0}", filepath);
			}
		}

		if (!std::filesystem::exists(filepath))
		{
			result = component.IsConvex ? CookConvexMesh(component.CollisionMesh, outData) : CookTriangleMesh(component.CollisionMesh, outData);

			if (result == CookingResult::Success)
			{
				// Serialize Collider Data
				uint32_t bufferSize = 0;
				uint32_t offset = 0;
				for (auto& colliderData : outData)
				{
					bufferSize += sizeof(uint32_t);
					bufferSize += colliderData.Size;
				}

				Buffer colliderBuffer;
				colliderBuffer.Allocate(bufferSize);
				for (auto& colliderData : outData)
				{
					colliderBuffer.Write((void*)&colliderData.Size, sizeof(uint32_t), offset);
					offset += sizeof(uint32_t);
					colliderBuffer.Write(colliderData.Data, colliderData.Size, offset);
					offset += colliderData.Size;
				}

				bool success = FileSystem::WriteBytes(filepath, colliderBuffer);
				colliderBuffer.Release();
				if (!success)
				{
					NR_CORE_ERROR("Failed to write collider to {0}", filepath.string());
					return CookingResult::Failure;
				}
			}
		}
		else
		{
			Buffer colliderBuffer = FileSystem::ReadBytes(filepath);
			if (colliderBuffer.Size > 0)
			{
				uint32_t offset = 0;
				const auto& meshAsset = component.CollisionMesh->GetMeshAsset();
				const auto& submeshes = meshAsset->GetSubmeshes();
				for (const auto& submesh : component.CollisionMesh->GetSubmeshes())
				{
					MeshColliderData& data = outData.emplace_back();
					data.Size = colliderBuffer.Read<uint32_t>(offset);
					offset += sizeof(uint32_t);
					data.Data = colliderBuffer.ReadBytes(data.Size, offset);
					offset += data.Size;
					data.Transform = submeshes[submesh].Transform;
				}

				colliderBuffer.Release();
				result = CookingResult::Success;
			}
		}

		if (result == CookingResult::Success && component.ProcessedMeshes.size() == 0)
		{
			for (const auto& colliderData : outData)
			{
				GenerateDebugMesh(component, colliderData);
			}
		}

		return result;
	}

	CookingResult CookingFactory::ForceCookMesh(MeshColliderComponent& component, const std::string& name, bool invalidateOld, std::vector<MeshColliderData>& outData)
	{
		NR_SCOPE_TIMER("CookingFactory::CookMesh");

		Utils::CreateCacheDirectoryIfNeeded();

		CookingResult result = CookingResult::Failure;
		std::filesystem::path filepath = Utils::GetCacheDirectory() / (name + (component.IsConvex ? "_convex.pxm" : "_tri.pxm"));
		if (invalidateOld)
		{
			component.ProcessedMeshes.clear();
			bool removedCached = std::filesystem::remove(filepath);
			if (!removedCached)
			{
				NR_CORE_WARN("Couldn't delete cached collider data for {0}", filepath);
			}
		}

		if (!std::filesystem::exists(filepath))
		{
			result = component.IsConvex ? CookConvexMesh(component.CollisionMesh, outData) : CookTriangleMesh(component.CollisionMesh, outData);

			if (result == CookingResult::Success)
			{
				// Serialize Collider Data
				uint32_t bufferSize = 0;
				uint32_t offset = 0;
				for (auto& colliderData : outData)
				{
					bufferSize += sizeof(uint32_t);
					bufferSize += colliderData.Size;
				}

				Buffer colliderBuffer;
				colliderBuffer.Allocate(bufferSize);
				for (auto& colliderData : outData)
				{
					colliderBuffer.Write((void*)&colliderData.Size, sizeof(uint32_t), offset);
					offset += sizeof(uint32_t);
					colliderBuffer.Write(colliderData.Data, colliderData.Size, offset);
					offset += colliderData.Size;
				}

				bool success = FileSystem::WriteBytes(filepath, colliderBuffer);
				colliderBuffer.Release();
				if (!success)
				{
					NR_CORE_ERROR("Failed to write collider to {0}", filepath.string());
					return CookingResult::Failure;
				}
			}
		}
		else
		{
			Buffer colliderBuffer = FileSystem::ReadBytes(filepath);
			if (colliderBuffer.Size > 0)
			{
				uint32_t offset = 0;
				const auto& meshAsset = component.CollisionMesh->GetMeshAsset();
				const auto& submeshes = meshAsset->GetSubmeshes();
				for (const auto& submesh : component.CollisionMesh->GetSubmeshes())
				{
					MeshColliderData& data = outData.emplace_back();
					data.Size = colliderBuffer.Read<uint32_t>(offset);
					offset += sizeof(uint32_t);
					data.Data = colliderBuffer.ReadBytes(data.Size, offset);
					offset += data.Size;
					data.Transform = submeshes[submesh].Transform;
				}

				colliderBuffer.Release();
				result = CookingResult::Success;
			}
		}

		if (result == CookingResult::Success && component.ProcessedMeshes.size() == 0)
		{
			for (const auto& colliderData : outData)
			{
				GenerateDebugMesh(component, colliderData);
			}
		}

		return result;
	}

	CookingResult CookingFactory::CookConvexMesh(const Ref<Mesh>& mesh, std::vector<MeshColliderData>& outData)
	{
		const auto& vertices = mesh->GetMeshAsset()->GetStaticVertices();
		const auto& indices = mesh->GetMeshAsset()->GetIndices();
		const auto& submeshes = mesh->GetMeshAsset()->GetSubmeshes();

		for (auto submeshIndex : mesh->GetSubmeshes())
		{
			const auto& submesh = submeshes[submeshIndex];

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
		const auto& vertices = mesh->GetMeshAsset()->GetStaticVertices();
		const auto& indices = mesh->GetMeshAsset()->GetIndices();
		const auto& submeshes = mesh->GetMeshAsset()->GetSubmeshes();

		for (auto submeshIndex : mesh->GetSubmeshes())
		{
			const auto& submesh = submeshes[submeshIndex];

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

		return CookingResult::Success;
	}

	void CookingFactory::GenerateDebugMesh(MeshColliderComponent& component, const MeshColliderData& colliderData)
	{
		physx::PxDefaultMemoryInputData input(colliderData.Data, colliderData.Size);
		std::vector<Vertex> vertices;
		std::vector<Index> indices;

		if (component.IsConvex)
		{
			physx::PxConvexMesh* convexMesh = PhysicsInternal::GetPhysicsSDK().createConvexMesh(input);

			// Magic values tada
			vertices.reserve(100);
			indices.reserve(50);

			const uint32_t nbPolygons = convexMesh->getNbPolygons();
			const physx::PxVec3* convexVertices = convexMesh->getVertices();
			const physx::PxU8* convexIndices = convexMesh->getIndexBuffer();
			uint32_t nbVertices = 0;
			uint32_t nbFaces = 0;
			uint32_t vertCounter = 0;
			uint32_t indexCounter = 0;

			for (uint32_t i = 0; i < nbPolygons; ++i)
			{
				physx::PxHullPolygon polygon;
				convexMesh->getPolygonData(i, polygon);
				nbVertices += polygon.mNbVerts;
				nbFaces += (polygon.mNbVerts - 2) * 3;
				uint32_t vI0 = vertCounter;

				for (uint32_t vI = 0; vI < polygon.mNbVerts; ++vI)
				{
					Vertex& v = vertices.emplace_back();
					v.Position = PhysicsUtils::FromPhysicsVector(convexVertices[convexIndices[polygon.mIndexBase + vI]]);
					++vertCounter;
				}

				for (uint32_t vI = 1; vI < uint32_t(polygon.mNbVerts) - 1; ++vI)
				{
					Index& index = indices.emplace_back();
					index.V1 = uint32_t(vI0);
					index.V2 = uint32_t(vI0 + vI + 1);
					index.V3 = uint32_t(vI0 + vI);
					++indexCounter;
				}
			}

			convexMesh->release();
		}
		else
		{
			physx::PxTriangleMesh* trimesh = PhysicsInternal::GetPhysicsSDK().createTriangleMesh(input);
			const uint32_t nbVerts = trimesh->getNbVertices();
			const physx::PxVec3* triangleVertices = trimesh->getVertices();
			const uint32_t nbTriangles = trimesh->getNbTriangles();
			const physx::PxU16* tris = (const physx::PxU16*)trimesh->getTriangles();

			vertices.reserve(nbVerts);
			indices.reserve(nbTriangles);

			for (uint32_t v = 0; v < nbVerts; ++v)
			{
				Vertex& v1 = vertices.emplace_back();
				v1.Position = PhysicsUtils::FromPhysicsVector(triangleVertices[v]);
			}

			for (uint32_t tri = 0; tri < nbTriangles; ++tri)
			{
				Index& index = indices.emplace_back();
				index.V1 = tris[3 * tri + 0];
				index.V2 = tris[3 * tri + 1];
				index.V3 = tris[3 * tri + 2];
			}

			trimesh->release();
		}

		Ref<MeshAsset> meshAsset = Ref<MeshAsset>::Create(vertices, indices, colliderData.Transform);
		component.ProcessedMeshes.push_back(Ref<Mesh>::Create(meshAsset));
	}
}