#include "nrpch.h"
#include "PhysicsMeshCache.h"

#include "NotRed/Asset/AssetManager.h"
#include "NotRed/Renderer/MeshFactory.h"

namespace NR
{
	void PhysicsMeshCache::Init()
	{
		mBoxMesh = MeshFactory::CreateBox(glm::vec3(1));
		mSphereMesh = MeshFactory::CreateSphere(0.5f);
		mCapsuleMesh = MeshFactory::CreateCapsule(0.5f, 1.0f);
	}

	const MeshColliderData& PhysicsMeshCache::GetMeshData(AssetHandle collisionMesh)
	{
		if (mMeshData.find(collisionMesh) != mMeshData.end())
		{
			return mMeshData.at(collisionMesh);
		}

		// Create/load collision mesh
		CookingResult result = CookingFactory::CookMesh(collisionMesh);
		NR_CORE_ASSERT(mMeshData.find(collisionMesh) != mMeshData.end());
		return mMeshData.at(collisionMesh);
	}

	// Debug meshes get created in CookingFactory::CookMesh
	Ref<Mesh> PhysicsMeshCache::GetDebugMesh(AssetHandle collisionMesh)
	{
		if (mDebugMeshes.find(collisionMesh) != mDebugMeshes.end())
		{
			return mDebugMeshes.at(collisionMesh);
		}

		return nullptr;
	}

	Ref<StaticMesh> PhysicsMeshCache::GetDebugStaticMesh(AssetHandle collisionMesh)
	{
		if (mDebugStaticMeshes.find(collisionMesh) != mDebugStaticMeshes.end())
		{
			return mDebugStaticMeshes.at(collisionMesh);
		}

		return nullptr;
	}

	bool PhysicsMeshCache::Exists(AssetHandle collisionMesh)
	{
		return mMeshData.find(collisionMesh) != mMeshData.end();
	}

	void PhysicsMeshCache::Clear()
	{
		mMeshData.clear();

		mDebugStaticMeshes.clear();
		mDebugMeshes.clear();
	}
}