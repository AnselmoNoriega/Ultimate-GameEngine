#pragma once

#include <PhysX/include/PxPhysicsAPI.h>

#include <glm/glm.hpp>

#include "NotRed/Scene/Components.h"
#include "PhysicsActorBase.h"


namespace NR
{
	enum class CollisionFlags : physx::PxU8
	{
		None,
		Sides,
		Above,
		Below
	};

	class PhysicsController : public PhysicsActorBase
	{
	public:
		PhysicsController(Entity entity);
		~PhysicsController();

		void SetHasGravity(bool hasGravity);
		void SetSimulationData(uint32_t layerId);
		void SetSlopeLimit(const float slopeLimitDeg);
		void SetStepOffset(const float stepOffset);

		void Move(const glm::vec3& displacement, float dt);

		glm::vec3 GetPosition() const;
		float GetSpeedDown() const;
		bool IsGrounded() const;
		CollisionFlags GetCollisionFlags() const;

		void Move(glm::vec3 displacement);
		void Jump(float jumpPower);

	private:
		void Update(float dt);
		void SynchronizeTransform() override;

	private:
		physx::PxController* mController = nullptr;
		physx::PxMaterial* mMaterial = nullptr;
		physx::PxControllerCollisionFlags mCollisionFlags = {};

		float mSpeedDown = 0.0f;      
		glm::vec3 mDisplacement = {}; 
		float mGravity = {};       
		
		bool mHasGravity = true;

	private:
		friend class PhysicsScene;
	};
}