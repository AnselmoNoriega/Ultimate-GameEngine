#include "nrpch.h"
#include "CookingFactory.h"

#include "PhysicsSystem.h"
#include "PhysicsInternal.h"

#include "NotRed/Asset/AssetManager.h"
#include "NotRed/Project/Project.h"
#include "NotRed/Math/Math.h"
#include "NotRed/Core/Timer.h"

#include <filesystem>

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

	// .nrpm file format
	struct NotRedPhysicsMesh
	{
		const char Header[9] = "NotRedPM";
		MeshColliderData::ColliderType Type;
		uint32_t SubmeshCount;
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
				std::filesystem::create_directories(cacheDirectory);
		}
	}

	CookingResult CookingFactory::CookMesh(AssetHandle collisionMesh, bool invalidateOld)
	{
		NR_SCOPE_TIMER("CookingFactory::CookMesh");

		Utils::CreateCacheDirectoryIfNeeded();

		auto& metadata = AssetManager::GetMetadata(collisionMesh);
		if (!metadata.IsValid())
		{
			NR_CORE_ERROR("Invalid mesh");
			return CookingResult::Failure;
		}

		CookingResult result = CookingResult::Failure;

		const auto& meshMetadata = AssetManager::GetMetadata(collisionMesh);
		NR_CORE_ASSERT(meshMetadata.Type == AssetType::StaticMesh || meshMetadata.Type == AssetType::Mesh);
		bool isStaticMesh = meshMetadata.Type == AssetType::StaticMesh;
		const Ref<Asset>& mesh = AssetManager::GetAsset<Asset>(collisionMesh);

		// name-handle.nrpm
		std::string filename = fmt::format("{0}-{1}.nrpm", metadata.FilePath.stem().string(), metadata.Handle);
		std::filesystem::path filepath = Utils::GetCacheDirectory() / filename;

		if (invalidateOld)
		{
			// TODO
			//component.ProcessedMeshes.clear();
			bool removedCached = std::filesystem::remove(filepath);
			if (!removedCached)
				NR_CORE_WARN("Couldn't delete cached collider data for {0}", filepath);
		}

		if (invalidateOld || !std::filesystem::exists(filepath))
		{
			MeshColliderData meshData;
			if (isStaticMesh)
				result = CookTriangleMesh(mesh.As<StaticMesh>(), meshData);
			else
				result = CookConvexMesh(mesh.As<Mesh>(), meshData);

			if (result == CookingResult::Success)
			{
				NotRedPhysicsMesh nrpm;
				nrpm.Type = isStaticMesh ? MeshColliderData::ColliderType::Triangle : MeshColliderData::ColliderType::Convex;
				nrpm.SubmeshCount = (uint32_t)meshData.Submeshes.size();

				std::ofstream stream(filepath, std::ios::binary | std::ios::trunc);
				if (!stream)
				{
					stream.close();
					NR_CORE_ERROR("Failed to write collider to {0}", filepath.string());
					return CookingResult::Failure;
				}

				stream.write((char*)&nrpm, sizeof(NotRedPhysicsMesh));
				for (auto& submesh : meshData.Submeshes)
				{
					stream.write((char*)glm::value_ptr(submesh.Transform), sizeof(submesh.Transform));
					stream.write((char*)&submesh.ColliderData.Size, sizeof(submesh.ColliderData.Size));
					stream.write((char*)submesh.ColliderData.Data, submesh.ColliderData.Size);
				}

				// Add to cache
				auto& meshCache = PhysicsSystem::GetMeshCache();
				meshCache.mMeshData[collisionMesh] = meshData;
			}

		}
		else
		{
			// Deserialize

			Buffer colliderBuffer = FileSystem::ReadBytes(filepath);
			NotRedPhysicsMesh& nrpm = *(NotRedPhysicsMesh*)colliderBuffer.Data;
			NR_CORE_VERIFY(strcmp(nrpm.Header, NotRedPhysicsMesh().Header) == 0);

			auto& meshCache = PhysicsSystem::GetMeshCache();
			MeshColliderData& meshData = meshCache.mMeshData[collisionMesh];
			meshData.Submeshes.resize(nrpm.SubmeshCount);

			uint8_t* buffer = colliderBuffer.As<uint8_t>() + sizeof(NotRedPhysicsMesh);
			for (uint32_t i = 0; i < nrpm.SubmeshCount; i++)
			{
				SubmeshColliderData& submeshData = meshData.Submeshes[i];

				// Transform
				memcpy(glm::value_ptr(submeshData.Transform), buffer, sizeof(glm::mat4));
				buffer += sizeof(glm::mat4);

				// Data
				uint32_t size = *(uint32_t*)buffer;
				buffer += sizeof(uint32_t);
				submeshData.ColliderData = Buffer::Copy(buffer, size);
				buffer += size;
			}

			colliderBuffer.Release();
			result = CookingResult::Success;
		}

		// Editor-only
		if (result == CookingResult::Success)
			GenerateDebugMesh(collisionMesh);

		return result;
	}

	CookingResult CookingFactory::CookConvexMesh(const Ref<Mesh>& mesh, MeshColliderData& outData)
	{
		const auto& vertices = mesh->GetMeshSource()->GetStaticVertices();
		const auto& indices = mesh->GetMeshSource()->GetIndices();
		const auto& submeshes = mesh->GetMeshSource()->GetSubmeshes();

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
				// TODO: free memory for existing meshes
				return PhysicsUtils::FromPhysicsCookingResult(result);
			}

			SubmeshColliderData& data = outData.Submeshes.emplace_back();
			data.ColliderData = Buffer::Copy(buf.getData(), buf.getSize());
			data.Transform = submesh.Transform;
		}

		return CookingResult::Success;
	}

	CookingResult CookingFactory::CookTriangleMesh(const Ref<StaticMesh>& staticMesh, MeshColliderData& outData)
	{
		const auto& vertices = staticMesh->GetMeshSource()->GetStaticVertices();
		const std::vector<Index>& indices = staticMesh->GetMeshSource()->GetIndices();

		const auto& submeshes = staticMesh->GetMeshSource()->GetSubmeshes();
		for (auto submeshIndex : staticMesh->GetSubmeshes())
		{
			const auto& submesh = submeshes[submeshIndex];

			physx::PxTriangleMeshDesc triangleDesc;
			triangleDesc.points.stride = sizeof(Vertex);
			triangleDesc.points.count = submesh.VertexCount;
			triangleDesc.points.data = &vertices[submesh.BaseVertex];
			triangleDesc.triangles.stride = sizeof(Index);
			triangleDesc.triangles.count = submesh.IndexCount / 3;
			triangleDesc.triangles.data = &indices[submesh.BaseIndex / 3];

			physx::PxDefaultMemoryOutputStream buf;
			physx::PxTriangleMeshCookingResult::Enum result;
			if (!PxCookTriangleMesh(sCookingData->CookingParameters, triangleDesc, buf, &result))
			{
				NR_CORE_ERROR("Failed to cook static mesh {0}", staticMesh->GetMeshSource()->GetFilePath());
				return PhysicsUtils::FromPhysicsCookingResult(result);
			}

			SubmeshColliderData& data = outData.Submeshes.emplace_back();
			data.ColliderData = Buffer::Copy(buf.getData(), buf.getSize());
			data.Transform = submesh.Transform;
		}

		return CookingResult::Success;
	}

	void CookingFactory::GenerateDebugMesh(AssetHandle collisionMesh)
	{
		const auto& meshMetadata = AssetManager::GetMetadata(collisionMesh);
		NR_CORE_ASSERT(meshMetadata.Type == AssetType::StaticMesh || meshMetadata.Type == AssetType::Mesh);
		bool isStaticMesh = meshMetadata.Type == AssetType::StaticMesh;

		if (isStaticMesh)
		{
			const MeshColliderData& meshData = PhysicsSystem::GetMeshCache().GetMeshData(collisionMesh);

			std::vector<Vertex> vertices;
			std::vector<Index> indices;
			std::vector<Submesh> submeshes;
			for (size_t i = 0; i < meshData.Submeshes.size(); i++)
			{
				const SubmeshColliderData& submeshData = meshData.Submeshes[i];

				physx::PxDefaultMemoryInputData input(submeshData.ColliderData.As<physx::PxU8>(), submeshData.ColliderData.Size);
				physx::PxTriangleMesh* trimesh = PhysicsInternal::GetPhysicsSDK().createTriangleMesh(input);

				const uint32_t nbVerts = trimesh->getNbVertices();
				const physx::PxVec3* triangleVertices = trimesh->getVertices();
				const uint32_t nbTriangles = trimesh->getNbTriangles();
				const physx::PxU16* tris = (const physx::PxU16*)trimesh->getTriangles();

				vertices.reserve(vertices.size() + nbVerts);
				indices.reserve(indices.size() + nbTriangles);

				Submesh& submesh = submeshes.emplace_back();
				submesh.BaseVertex = static_cast<uint32_t>(vertices.size());
				submesh.VertexCount = nbVerts;
				submesh.BaseIndex = static_cast<uint32_t>(indices.size()) * 3;
				submesh.IndexCount = nbTriangles * 3;
				submesh.MaterialIndex = 0;
				submesh.Transform = submeshData.Transform;

				for (uint32_t v = 0; v < nbVerts; v++)
				{
					Vertex& v1 = vertices.emplace_back();
					v1.Position = PhysicsUtils::FromPhysicsVector(triangleVertices[v]);
				}

				for (uint32_t tri = 0; tri < nbTriangles; tri++)
				{
					Index& index = indices.emplace_back();
					index.V1 = tris[3 * tri + 0];
					index.V2 = tris[3 * tri + 1];
					index.V3 = tris[3 * tri + 2];
				}

				trimesh->release();
			}

			Ref<MeshSource> meshAsset = Ref<MeshSource>::Create(vertices, indices, submeshes);
			PhysicsSystem::GetMeshCache().mDebugStaticMeshes[collisionMesh] = Ref<StaticMesh>::Create(meshAsset);
		}
		else
		{
			const MeshColliderData& meshData = PhysicsSystem::GetMeshCache().GetMeshData(collisionMesh);

			std::vector<Vertex> vertices;
			std::vector<Index> indices;
			std::vector<Submesh> submeshes;

			for (size_t i = 0; i < meshData.Submeshes.size(); i++)
			{
				const SubmeshColliderData& submeshData = meshData.Submeshes[i];

				physx::PxDefaultMemoryInputData input(submeshData.ColliderData.As<physx::PxU8>(), submeshData.ColliderData.Size);
				physx::PxConvexMesh* convexMesh = PhysicsInternal::GetPhysicsSDK().createConvexMesh(input);

				// Based On: https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Source/ThirdParty/Physics3/NvCloth/samples/SampleBase/renderer/ConvexRenderMesh.cpp
				const uint32_t nbPolygons = convexMesh->getNbPolygons();
				const physx::PxVec3* convexVertices = convexMesh->getVertices();
				const physx::PxU8* convexIndices = convexMesh->getIndexBuffer();

				uint32_t nbVertices = 0;
				uint32_t nbFaces = 0;
				uint32_t vertCounter = 0;
				uint32_t indexCounter = 0;

				Submesh& submesh = submeshes.emplace_back();
				submesh.BaseVertex = static_cast<uint32_t>(vertices.size());
				submesh.BaseIndex = static_cast<uint32_t>(indices.size()) * 3;

				for (uint32_t i = 0; i < nbPolygons; i++)
				{
					physx::PxHullPolygon polygon;
					convexMesh->getPolygonData(i, polygon);
					nbVertices += polygon.mNbVerts;
					nbFaces += (polygon.mNbVerts - 2) * 3;

					uint32_t vI0 = vertCounter;
					for (uint32_t vI = 0; vI < polygon.mNbVerts; vI++)
					{
						Vertex& v = vertices.emplace_back();
						v.Position = PhysicsUtils::FromPhysicsVector(convexVertices[convexIndices[polygon.mIndexBase + vI]]);
						vertCounter++;
					}

					for (uint32_t vI = 1; vI < uint32_t(polygon.mNbVerts) - 1; vI++)
					{
						Index& index = indices.emplace_back();
						index.V1 = uint32_t(vI0);
						index.V2 = uint32_t(vI0 + vI + 1);
						index.V3 = uint32_t(vI0 + vI);
						indexCounter++;
					}
				}

				submesh.VertexCount = vertCounter;
				submesh.IndexCount = indexCounter * 3;
				submesh.MaterialIndex = 0;
				submesh.Transform = submeshData.Transform;

				convexMesh->release();
			}

			Ref<MeshSource> meshAsset = Ref<MeshSource>::Create(vertices, indices, submeshes);
			PhysicsSystem::GetMeshCache().mDebugMeshes[collisionMesh] = Ref<Mesh>::Create(meshAsset);
		}

	}

}