#include "nrpch.h"
#include "PhysicsUtil.h"

#include "NotRed/Script/ScriptEngine.h"

namespace NR
{
	physx::PxTransform ToPhysicsTransform(const glm::mat4& matrix)
	{
		physx::PxQuat r = ToPhysicsQuat(glm::normalize(glm::toQuat(matrix)));
		physx::PxVec3 p = ToPhysicsVector(glm::vec3(matrix[3]));
		return physx::PxTransform(p, r);
	}

	physx::PxMat44 ToPhysicsMatrix(const glm::mat4& matrix)
	{
		return *(physx::PxMat44*)&matrix;
	}

	physx::PxVec3 ToPhysicsVector(const glm::vec3& vector)
	{
		return physx::PxVec3(vector.x, vector.y, vector.z);
	}

	physx::PxVec4 ToPhysicsVector(const glm::vec4& vector)
	{
		return physx::PxVec4(vector.x, vector.y, vector.z, vector.w);
	}

	physx::PxQuat ToPhysicsQuat(const glm::quat& quat)
	{
		return physx::PxQuat(quat.x, quat.y, quat.z, quat.w);
	}

	glm::mat4 FromPhysicsTransform(const physx::PxTransform& transform)
	{
		glm::quat rotation = FromPhysicsQuat(transform.q);
		glm::vec3 position = FromPhysicsVector(transform.p);
		return glm::translate(glm::mat4(1.0F), position) * glm::toMat4(rotation);
	}

	glm::mat4 FromPhysicsMatrix(const physx::PxMat44& matrix)
	{
		return *(glm::mat4*)&matrix;
	}

	glm::vec3 FromPhysicsVector(const physx::PxVec3& vector)
	{
		return glm::vec3(vector.x, vector.y, vector.z);
	}

	glm::vec4 FromPhysicsVector(const physx::PxVec4& vector)
	{
		return glm::vec4(vector.x, vector.y, vector.z, vector.w);
	}

	glm::quat FromPhysicsQuat(const physx::PxQuat& quat)
	{
		return glm::quat(quat.w, quat.x, quat.y, quat.z);
	}

	physx::PxFilterFlags FilterShader(
		physx::PxFilterObjectAttributes attributes0,
		physx::PxFilterData filterData0,
		physx::PxFilterObjectAttributes attributes1,
		physx::PxFilterData filterData1,
		physx::PxPairFlags& pairFlags,
		const void* constantBlock,
		physx::PxU32 constantBlockSize
	) 
	{
		if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1))
		{
			pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
			return physx::PxFilterFlag::eDEFAULT;
		}

		pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;

		if ((filterData0.word0 & filterData1.word1) || (filterData1.word0 & filterData0.word1))
		{
			pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;
			pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_LOST;
		}

		return physx::PxFilterFlag::eDEFAULT;
	}

	void ContactListener::onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count)
	{
		PX_UNUSED(constraints);
		PX_UNUSED(count);
	}

	void ContactListener::onWake(physx::PxActor** actors, physx::PxU32 count)
	{
		PX_UNUSED(actors);
		PX_UNUSED(count);
	}

	void ContactListener::onSleep(physx::PxActor** actors, physx::PxU32 count)
	{
		PX_UNUSED(actors);
		PX_UNUSED(count);
	}

	void ContactListener::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
	{
		Entity& a = *(Entity*)pairHeader.actors[0]->userData;
		Entity& b = *(Entity*)pairHeader.actors[1]->userData;

		if (pairs->flags == physx::PxContactPairFlag::eACTOR_PAIR_HAS_FIRST_TOUCH)
		{
			if (ScriptEngine::IsEntityModuleValid(a)) 
			{
				ScriptEngine::CollisionBegin(a);
			}
			if (ScriptEngine::IsEntityModuleValid(b)) 
			{
				ScriptEngine::CollisionBegin(b);
			}
		}
		else if (pairs->flags == physx::PxContactPairFlag::eACTOR_PAIR_LOST_TOUCH)
		{
			if (ScriptEngine::IsEntityModuleValid(a)) 
			{
				ScriptEngine::CollisionEnd(a);
			}
			if (ScriptEngine::IsEntityModuleValid(b)) 
			{
				ScriptEngine::CollisionEnd(b);
			}
		}
	}

	void ContactListener::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
	{
		Entity& a = *(Entity*)pairs->triggerActor->userData;
		Entity& b = *(Entity*)pairs->otherActor->userData;

		if (pairs->status == physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
		{
			if (ScriptEngine::IsEntityModuleValid(a)) 
			{
				ScriptEngine::TriggerBegin(a);
			}
			if (ScriptEngine::IsEntityModuleValid(b)) 
			{
				ScriptEngine::TriggerBegin(b);
			}
		}
		else if (pairs->status == physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
		{
			if (ScriptEngine::IsEntityModuleValid(a)) 
			{
				ScriptEngine::TriggerEnd(a);
			}
			if (ScriptEngine::IsEntityModuleValid(b)) 
			{
				ScriptEngine::TriggerEnd(b);
			}
		}
	}

	void ContactListener::onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count)
	{
		PX_UNUSED(bodyBuffer);
		PX_UNUSED(poseBuffer);
		PX_UNUSED(count);
	}

}