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

		void SetHasGravity(bool hasGravity);
		void SetSimulationData(uint32_t layerId);
		void SetSlopeLimit(const float slopeLimitDeg);
		void SetStepOffset(const float stepOffset);

		void Move(const glm::vec3& displacement, float dt);

		glm::vec3 GetPosition() const;
		const glm::vec3& GetVelocity() const;
		bool IsGrounded() const;
		CollisionFlags GetCollisionFlags() const;

		void Move(glm::vec3 displacement);

	private:
		void Update(float dt);
		void SynchronizeTransform();

	private:
		Entity mEntity;
		physx::PxController* mController = nullptr;
		physx::PxMaterial* mMaterial = nullptr;
		physx::PxControllerCollisionFlags mCollisionFlags = {};
		
		glm::vec3 mVelocity = {};         // velocity of controller at last update
		glm::vec3 mDisplacement = {};     // displacement (if any) for next update (comes from Move() calls)
		glm::vec3 mGravity = {};          // gravity for this controller (initialized to be opposite of controllers "up" vector,
		
		bool mHasGravity = true;

	private:
		friend class PhysicsScene;
	};
}