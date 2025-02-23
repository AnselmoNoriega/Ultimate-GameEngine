#pragma once

#include "PhysicsUtils.h"

#include "NotRed/Core/Core.h"
#include "NotRed/Renderer/Mesh.h"

namespace NR
{
	struct SubmeshColliderData
	{
		Buffer ColliderData;
		glm::mat4 Transform;
	};

	struct MeshColliderData
	{
		std::vector<SubmeshColliderData> Submeshes;
		enum class ColliderType : uint8_t { Triangle, Convex };
		ColliderType Type;
	};

	class CookingFactory
	{
	public:
		static void Initialize();
		static void Shutdown();

		static CookingResult ForceCookMesh(AssetHandle collisionMesh);
		static CookingResult CookMesh(AssetHandle collisionMesh, bool invalidateOld = false);

		static CookingResult CookConvexMesh(const Ref<Mesh>& mesh, MeshColliderData& outData);
		static CookingResult CookTriangleMesh(const Ref<StaticMesh>& staticMesh, MeshColliderData& outData);
		
	private:
		static void GenerateDebugMesh(AssetHandle collisionMesh);

	private:
		static std::vector<MeshColliderData> mDefaultOutData;
	};
}