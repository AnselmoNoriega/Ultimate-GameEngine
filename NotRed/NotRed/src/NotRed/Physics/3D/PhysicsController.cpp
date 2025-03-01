#include "nrpch.h"
#include "PhysicsController.h"

#include "PhysicsManager.h"
#include "PhysicsLayer.h"
#include "PhysicsUtils.h"

namespace NR
{
	PhysicsController::PhysicsController(Entity entity)
		: PhysicsActorBase(PhysicsActorBase::Type::Controller, entity)
		, mGravity(glm::length(PhysicsManager::GetSettings().Gravity))
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

	void PhysicsController::SetHasGravity(bool hasGravity)
	{
		mHasGravity = hasGravity;
	}

	void PhysicsController::SetSimulationData(uint32_t layerId)
	{
		const PhysicsLayer& layerInfo = PhysicsLayerManager::GetLayer(layerId);

		if (layerInfo.CollidesWith == 0)
			return;

		physx::PxFilterData filterData;
		filterData.word0 = layerInfo.BitValue;
		filterData.word1 = layerInfo.CollidesWith;
		filterData.word2 = 1;

		const auto actor = mController->getActor();
		NR_CORE_ASSERT(actor && actor->getNbShapes() == 1);
		physx::PxShape* shape;
		actor->getShapes(&shape, 1);
		shape->setSimulationFilterData(filterData);
	}

	void PhysicsController::SetSlopeLimit(const float slopeLimitDeg)
	{
		if (mController)
		{
			mController->setSlopeLimit(std::max(0.0f, cos(glm::radians(slopeLimitDeg))));
		}
	}

	void PhysicsController::SetStepOffset(const float stepOffset)
	{
		if (mController)
		{
			mController->setStepOffset(stepOffset);
		}
	}

	void PhysicsController::Move(glm::vec3 displacement)
	{
		mDisplacement += displacement;
	}

	void PhysicsController::Jump(float jumpPower)
	{
		mSpeedDown = -1.0f * jumpPower;
	}

	glm::vec3 PhysicsController::GetPosition() const 
	{
		const auto& pxPos = mController->getPosition();
		glm::vec3 pos = { pxPos.x, pxPos.y, pxPos.z };

		if (mEntity.HasComponent<CapsuleColliderComponent>())
		{
			const auto& ccc = mEntity.GetComponent<CapsuleColliderComponent>();
			pos -= ccc.Offset;
		}
		else if (mEntity.HasComponent<BoxColliderComponent>())
		{
			const auto& bcc = mEntity.GetComponent<BoxColliderComponent>();
			pos -= bcc.Offset;
		}

		return pos;
	}

	float PhysicsController::GetSpeedDown() const
	{
		return mSpeedDown;
	}

	bool PhysicsController::IsGrounded() const
	{
		return mCollisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_DOWN;
	}

	CollisionFlags PhysicsController::GetCollisionFlags() const
	{
		return static_cast<CollisionFlags>((physx::PxU8)mCollisionFlags);
	}

	void PhysicsController::Update(float dt)
	{
		physx::PxControllerFilters filters;

		if (mHasGravity)
		{
			mSpeedDown += mGravity * dt;
		}

		glm::vec3 displacement = mDisplacement - PhysicsUtils::FromPhysicsVector(mController->getUpDirection()) * mSpeedDown * dt;

		mCollisionFlags = mController->move(PhysicsUtils::ToPhysicsVector(displacement), 0.0, static_cast<physx::PxF32>(dt), filters);

		if (IsGrounded())
		{
			mSpeedDown = mGravity * 0.01f; // setting mVelocity back to zero here would be more technically correct,
		}

		mDisplacement = {};
	}

	void PhysicsController::SynchronizeTransform()
	{
		TransformComponent& transform = mEntity.Transform();
		transform.Translation = GetPosition();
	}
}