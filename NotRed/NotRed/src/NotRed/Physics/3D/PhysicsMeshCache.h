#pragma once

#include "NotRed/Asset/Asset.h"
#include "CookingFactory.h"

#include <map>

namespace NR
{
	class PhysicsMeshCache
	{
	public:
		const MeshColliderData& GetMeshData(AssetHandle collisionMesh);
		Ref<Mesh> GetDebugMesh(AssetHandle collisionMesh);
		Ref<StaticMesh> GetDebugStaticMesh(AssetHandle collisionMesh);

		bool Exists(AssetHandle collisionMesh);

		void Clear();
	private:
		std::map<AssetHandle, MeshColliderData> mMeshData;

		// Editor-only
		std::map<AssetHandle, Ref<StaticMesh>> mDebugStaticMeshes;
		std::map<AssetHandle, Ref<Mesh>> mDebugMeshes;

		friend class CookingFactory;
	};

}