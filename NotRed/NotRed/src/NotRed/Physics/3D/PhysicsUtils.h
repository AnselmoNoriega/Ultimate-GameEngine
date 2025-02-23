#pragma once

#include <PhysX/include/PxPhysicsAPI.h>
#include <glm/gtc/type_ptr.hpp>

#include "NotRed/Scene/Components.h"
#include "PhysicsTypes.h"

namespace NR
{
	namespace PhysicsUtils
	{
		physx::PxTransform ToPhysicsTransform(const TransformComponent& transform);
		physx::PxTransform ToPhysicsTransform(const glm::mat4& transform);
		physx::PxTransform ToPhysicsTransform(const glm::vec3& translation, const glm::vec3& rotation);
		physx::PxMat44 ToPhysicsMatrix(const glm::mat4& matrix);
		const physx::PxVec3& ToPhysicsVector(const glm::vec3& vector);
		const physx::PxVec4& ToPhysicsVector(const glm::vec4& vector);
		physx::PxExtendedVec3 ToPhysicsExtendedVector(const glm::vec3& vector);
		physx::PxQuat ToPhysicsQuat(const glm::quat& quat);

		glm::mat4 FromPhysicsTransform(const physx::PxTransform& transform);
		glm::mat4 FromPhysicsMatrix(const physx::PxMat44& matrix);
		glm::vec3 FromPhysicsVector(const physx::PxVec3& vector);
		glm::vec4 FromPhysicsVector(const physx::PxVec4& vector);
		glm::quat FromPhysicsQuat(const physx::PxQuat& quat);

		CookingResult FromPhysicsCookingResult(physx::PxConvexMeshCookingResult::Enum cookingResult);
		CookingResult FromPhysicsCookingResult(physx::PxTriangleMeshCookingResult::Enum cookingResult);
		
		physx::PxBroadPhaseType::Enum ToPhysicsBroadphaseType(BroadphaseType type);
		physx::PxFrictionType::Enum ToPhysicsFrictionType(FrictionType type);
	}
}