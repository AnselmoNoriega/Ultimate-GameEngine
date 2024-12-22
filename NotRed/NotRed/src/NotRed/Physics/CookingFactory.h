#pragma once

#include <PhysX/include/PxPhysicsAPI.h>

#include "PhysicsUtils.h"

#include "NotRed/Core/Core.h"
#include "NotRed/Renderer/Mesh.h"

namespace NR
{
	struct MeshColliderData
	{
		byte* Data;
		glm::mat4 Transform;
		uint32_t Size;
	};

	class CookingFactory
	{
	public:
		static void Initialize();
		static void Shutdown();

		static CookingResult CookMesh(MeshColliderComponent& component, bool isConvex, std::vector<MeshColliderData>& outData = mDefaultOutData);

		static CookingResult CookConvexMesh(const Ref<Mesh>& mesh, std::vector<MeshColliderData>& outData);
		static CookingResult CookTriangleMesh(const Ref<Mesh>& mesh, std::vector<MeshColliderData>& outData);

	private:
		static void GenerateDebugMesh(MeshColliderComponent& component, const MeshColliderData& colliderData, bool isConvex);

	private:
		static std::vector<MeshColliderData> mDefaultOutData;
	};
}