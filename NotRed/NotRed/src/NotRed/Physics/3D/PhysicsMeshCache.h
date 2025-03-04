#pragma once

#include "NotRed/Asset/Asset.h"
#include "CookingFactory.h"

#include <map>

namespace NR
{
	class PhysicsMeshCache
	{
	public:
		void Init();

		const MeshColliderData& GetMeshData(AssetHandle collisionMesh);
		Ref<Mesh> GetDebugMesh(AssetHandle collisionMesh);
		Ref<StaticMesh> GetDebugStaticMesh(AssetHandle collisionMesh);

		AssetHandle GetBoxDebugMesh() const { return mBoxMesh; }
		AssetHandle GetSphereDebugMesh() const { return mSphereMesh; }
		AssetHandle GetCapsuleDebugMesh() const { return mCapsuleMesh; }

		bool Exists(AssetHandle collisionMesh);

		void Clear();
	private:
		std::map<AssetHandle, MeshColliderData> mMeshData;

		// Editor-only
		std::map<AssetHandle, Ref<StaticMesh>> mDebugStaticMeshes;
		std::map<AssetHandle, Ref<Mesh>> mDebugMeshes;

		AssetHandle mBoxMesh = 0, mSphereMesh = 0, mCapsuleMesh = 0;

		friend class CookingFactory;
	};

}