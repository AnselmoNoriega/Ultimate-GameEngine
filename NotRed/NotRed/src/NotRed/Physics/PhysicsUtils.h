#pragma once

#include <PhysX/include/PxPhysicsAPI.h>
#include <glm/gtc/type_ptr.hpp>

#include "NotRed/Scene/Components.h"

namespace NR
{
	enum class CookingResult
	{
		Success,

		ZeroAreaTestFailed,
		PolygonLimitReached,

		LargeTriangle,

		Failure
	};

	enum class ForceMode : uint8_t
	{
		Force,
		Impulse,
		VelocityChange,
		Acceleration
	};

	enum class ActorLockFlag
	{
		PositionX = 1 << 0, PositionY = 1 << 1, PositionZ = 1 << 2, 
		Position = PositionX | PositionY | PositionZ,
		
		RotationX = 1 << 3, RotationY = 1 << 4, RotationZ = 1 << 5, 
		Rotation = RotationX | RotationY | RotationZ
	};

	namespace PhysicsUtils
	{
		physx::PxTransform ToPhysicsTransform(const TransformComponent& transform);
		physx::PxTransform ToPhysicsTransform(const glm::mat4& transform);
		physx::PxTransform ToPhysicsTransform(const glm::vec3& translation, const glm::vec3& rotation);
		physx::PxMat44 ToPhysicsMatrix(const glm::mat4& matrix);
		const physx::PxVec3& ToPhysicsVector(const glm::vec3& vector);
		const physx::PxVec4& ToPhysicsVector(const glm::vec4& vector);
		physx::PxQuat ToPhysicsQuat(const glm::quat& quat);

		glm::mat4 FromPhysicsTransform(const physx::PxTransform& transform);
		glm::mat4 FromPhysicsMatrix(const physx::PxMat44& matrix);
		glm::vec3 FromPhysicsVector(const physx::PxVec3& vector);
		glm::vec4 FromPhysicsVector(const physx::PxVec4& vector);
		glm::quat FromPhysicsQuat(const physx::PxQuat& quat);

		CookingResult FromPhysicsCookingResult(physx::PxConvexMeshCookingResult::Enum cookingResult);
		CookingResult FromPhysicsCookingResult(physx::PxTriangleMeshCookingResult::Enum cookingResult);
	}
}