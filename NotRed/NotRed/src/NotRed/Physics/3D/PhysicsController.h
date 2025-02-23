#pragma once

#include <PhysX/include/PxPhysicsAPI.h>

#include <glm/glm.hpp>

#include "NotRed/Scene/Components.h"
#include "NotRed/Scene/Entity.h"


namespace NR
{
	class PhysicsController : public RefCounted
	{
	public:
		PhysicsController(Entity entity);
		~PhysicsController();

		Entity GetEntity() const { return mEntity; }

		void SetSlopeLimit(const float slopeLimit);
		void SetStepOffset(const float stepOffset);

		void Move(const glm::vec3& displacement, float dt);

		glm::vec3 GetPosition() const;

	private:
		Entity mEntity;
		physx::PxController* mController = nullptr;
		physx::PxMaterial* mMaterial = nullptr;

	private:
		friend class PhysicsScene;
	};
}