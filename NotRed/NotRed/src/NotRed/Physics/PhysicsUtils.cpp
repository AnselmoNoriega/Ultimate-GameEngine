#include "nrpch.h"
#include "PhysicsUtils.h"

#include "NotRed/Math/Math.h"

namespace NR
{
	namespace PhysicsUtils
	{
		physx::PxTransform ToPhysicsTransform(const TransformComponent& transform)
		{
			physx::PxQuat r = ToPhysicsQuat(glm::quat(transform.Rotation));
			physx::PxVec3 p = ToPhysicsVector(transform.Translation);
			return physx::PxTransform(p, r);
		}

		physx::PxTransform ToPhysicsTransform(const glm::mat4& transform)
		{
			glm::vec3 translation, rotation, scale;
			Math::DecomposeTransform(transform, translation, rotation, scale);

			physx::PxQuat r = ToPhysicsQuat(glm::quat(rotation));
			physx::PxVec3 p = ToPhysicsVector(translation);
			return physx::PxTransform(p, r);
		}

		physx::PxTransform ToPhysicsTransform(const glm::vec3& translation, const glm::vec3& rotation)
		{
			return physx::PxTransform(ToPhysicsVector(translation), ToPhysicsQuat(glm::quat(rotation)));
		}

		physx::PxTransform ToPhyscsTransform(const glm::vec3& translation, const glm::vec3& rotation)
		{
			return physx::PxTransform(ToPhysicsVector(translation), ToPhysicsQuat(glm::quat(rotation)));
		}

		physx::PxMat44 ToPhysicsMatrix(const glm::mat4& matrix) { return *(physx::PxMat44*)&matrix; }
		const physx::PxVec3& ToPhysicsVector(const glm::vec3& vector) { return *(physx::PxVec3*)&vector; }
		const physx::PxVec4& ToPhysicsVector(const glm::vec4& vector) { return *(physx::PxVec4*)&vector; }
		physx::PxQuat ToPhysicsQuat(const glm::quat& quat) { return physx::PxQuat(quat.x, quat.y, quat.z, quat.w); }

		glm::mat4 FromPhysicsTransform(const physx::PxTransform& transform)
		{
			glm::quat rotation = FromPhysicsQuat(transform.q);
			glm::vec3 position = FromPhysicsVector(transform.p);
			return glm::translate(glm::mat4(1.0F), position) * glm::toMat4(rotation);
		}

		glm::mat4 FromPhysicsMatrix(const physx::PxMat44& matrix) { return *(glm::mat4*)&matrix; }
		glm::vec3 FromPhysicsVector(const physx::PxVec3& vector) { return *(glm::vec3*)&vector; }
		glm::vec4 FromPhysicsVector(const physx::PxVec4& vector) { return *(glm::vec4*)&vector; }
		glm::quat FromPhysicsQuat(const physx::PxQuat& quat) { return *(glm::quat*)&quat; }

		CookingResult FromPhysicsCookingResult(physx::PxConvexMeshCookingResult::Enum cookingResult)
		{
			switch (cookingResult)
			{
			case physx::PxConvexMeshCookingResult::eSUCCESS: return CookingResult::Success;
			case physx::PxConvexMeshCookingResult::eZERO_AREA_TEST_FAILED: return CookingResult::ZeroAreaTestFailed;
			case physx::PxConvexMeshCookingResult::ePOLYGONS_LIMIT_REACHED: return CookingResult::PolygonLimitReached;
			case physx::PxConvexMeshCookingResult::eFAILURE: return CookingResult::Failure;
			default:
				return CookingResult::Failure;
			}
		}

		CookingResult FromPhysicsCookingResult(physx::PxTriangleMeshCookingResult::Enum cookingResult)
		{
			switch (cookingResult)
			{
			case physx::PxTriangleMeshCookingResult::eSUCCESS: return CookingResult::Success;
			case physx::PxTriangleMeshCookingResult::eLARGE_TRIANGLE: return CookingResult::LargeTriangle;
			case physx::PxTriangleMeshCookingResult::eFAILURE: return CookingResult::Failure;
			}
			return CookingResult::Failure;
		}
	}
}