#pragma once

#include <PhysX/include/PxPhysicsAPI.h>

#include <glm/glm.hpp>

#include "NotRed/Scene/Components.h"
#include "NotRed/Scene/Entity.h"


namespace NR
{
	enum class CollisionFlags : physx::PxU8
	{
		None,
		Sides,
		Above,
		Below
	};

	class PhysicsController : public RefCounted
	{
	public:
		PhysicsController(Entity entity);
		~PhysicsController();

		Entity GetEntity() const { return mEntity; }

		void SetSimulationData(uint32_t layerId);
		void SetSlopeLimit(const float slopeLimitDeg);
		void SetStepOffset(const float stepOffset);

		void Move(const glm::vec3& displacement, float dt);

		glm::vec3 GetPosition() const;

		bool IsGrounded() const
		{
			return mCollisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_DOWN;
		}

		CollisionFlags GetCollisionFlags() const
		{
			return static_cast<CollisionFlags>((physx::PxU8)mCollisionFlags);
		}

	private:
		Entity mEntity;
		physx::PxController* mController = nullptr;
		physx::PxMaterial* mMaterial = nullptr;
		physx::PxControllerCollisionFlags mCollisionFlags = {};

	private:
		friend class PhysicsScene;
	};
}