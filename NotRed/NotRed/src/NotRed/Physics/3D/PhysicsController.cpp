#include "nrpch.h"
#include "PhysicsController.h"
#include "PhysicsUtils.h"

namespace NR
{
	PhysicsController::PhysicsController(Entity entity)
		: mEntity(entity), mController(nullptr)
	{
	}

	PhysicsController::~PhysicsController()
	{
		if (mController)
		{
			mController->release();
		}

		if (mMaterial)
		{
			mMaterial->release();
		}
	}

	void PhysicsController::SetSlopeLimit(const float slopeLimit)
	{
		if (mController)
		{
			mController->setSlopeLimit(slopeLimit);
		}
	}

	void PhysicsController::SetStepOffset(const float stepOffset)
	{
		if (mController)
		{
			mController->setStepOffset(stepOffset);
		}
	}

	void PhysicsController::Move(const glm::vec3& displacement, float dt)
	{
		physx::PxControllerFilters filters;

		mController->move(PhysicsUtils::ToPhysicsVector(displacement), 0.0, static_cast<physx::PxF32>(dt), filters);
	}

	glm::vec3 PhysicsController::GetPosition() const 
	{
		auto pxPos = mController->getPosition();
		glm::vec3 pos = { pxPos.x, pxPos.y, pxPos.z };
		const auto& ccc = mEntity.GetComponent<CapsuleColliderComponent>();
		pos -= ccc.Offset;
		return pos;
	}
}