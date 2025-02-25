#include "nrpch.h"
#include "PhysicsController.h"
#include "PhysicsLayer.h"
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

	void PhysicsController::Move(const glm::vec3& displacement, float dt)
	{
		physx::PxControllerFilters filters;

		mCollisionFlags = mController->move(PhysicsUtils::ToPhysicsVector(displacement), 0.0, static_cast<physx::PxF32>(dt), filters);
	}

	glm::vec3 PhysicsController::GetPosition() const 
	{
		const auto& pxPos = mController->getPosition();
		glm::vec3 pos = { pxPos.x, pxPos.y, pxPos.z };
		const auto& ccc = mEntity.GetComponent<CapsuleColliderComponent>();
		pos -= ccc.Offset;
		return pos;
	}
}