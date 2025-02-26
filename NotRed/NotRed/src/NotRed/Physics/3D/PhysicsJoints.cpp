#include "nrpch.h"
#include "PhysicsJoints.h"

#include <Physics/include/extensions/PxFixedJoint.h>

#include "PhysicsManager.h"
#include "PhysicsInternal.h"
#include "NotRed/Script/ScriptEngine.h"

namespace NR
{
	physx::PxTransform JointBase::GetLocalFrame(physx::PxRigidActor& attachActor)
	{
		auto scene = Scene::GetScene(mEntity.GetSceneID());

		const auto& worldTransform = scene->GetWorldSpaceTransform(mEntity);

		glm::quat rotationQuat = glm::quat(worldTransform.Rotation);
		glm::vec3 globalNormal = rotationQuat * glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3 globalAxis = rotationQuat * glm::vec3(0.0f, 1.0f, 0.0f);

		physx::PxVec3 localAnchor = attachActor.getGlobalPose().transformInv(PhysicsUtils::ToPhysicsVector(worldTransform.Translation));
		physx::PxVec3 localNormal = attachActor.getGlobalPose().rotateInv(PhysicsUtils::ToPhysicsVector(globalNormal));
		physx::PxVec3 localAxis = attachActor.getGlobalPose().rotateInv(PhysicsUtils::ToPhysicsVector(globalAxis));

		physx::PxMat33 rot(localAxis, localNormal, localAxis.cross(localNormal));
		physx::PxTransform localFrame;
		localFrame.p = localAnchor;
		localFrame.q = physx::PxQuat(rot);
		localFrame.q.normalize();

		return localFrame;
	}

	FixedJoint::FixedJoint(Entity entity)
		: JointBase(entity, JointType::Fixed)
	{
		auto physicsScene = PhysicsManager::GetScene();

		const auto& fixedJointComponent = entity.GetComponent<FixedJointComponent>();
		Entity other = physicsScene->GetEntityScene()->FindEntityByID(fixedJointComponent.ConnectedEntity);
		NR_CORE_VERIFY(other, "FixedJointComponent requires a connected entity to be set!");
		NR_CORE_VERIFY(entity.HasComponent<RigidBodyComponent>() || other.HasComponent<RigidBodyComponent>());

		auto& actor0 = physicsScene->GetActor(entity)->GetPhysicsActor();
		auto& actor1 = physicsScene->GetActor(other)->GetPhysicsActor();

		physx::PxTransform localFrame0 = GetLocalFrame(actor0);
		physx::PxTransform localFrame1 = GetLocalFrame(actor1);

		mJoint = physx::PxFixedJointCreate(PhysicsInternal::GetPhysicsSDK(), &actor0, localFrame0, &actor1, localFrame1);
		NR_CORE_ASSERT(mJoint, "Failed to create FixedJoint!");

		if (fixedJointComponent.IsBreakable)
		{
			mJoint->setBreakForce(fixedJointComponent.BreakForce, fixedJointComponent.BreakTorque);
		}
		else
		{
			mJoint->setBreakForce(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
		}

		mJoint->setConstraintFlag(physx::PxConstraintFlag::eCOLLISION_ENABLED, fixedJointComponent.EnableCollision);
		mJoint->setConstraintFlag(physx::PxConstraintFlag::eDISABLE_PREPROCESSING, !fixedJointComponent.EnablePreProcessing);
	}

	FixedJoint::~FixedJoint()
	{
		Release();
	}

	void FixedJoint::SetConnectedEntity(Entity other)
	{
		NR_CORE_VERIFY(mJoint);

		auto physicsScene = PhysicsManager::GetScene();

		auto& actor0 = physicsScene->GetActor(mEntity)->GetPhysicsActor();
		auto& actor1 = physicsScene->GetActor(other)->GetPhysicsActor();

		physx::PxTransform localFrame0 = GetLocalFrame(actor0);
		physx::PxTransform localFrame1 = GetLocalFrame(actor1);

		mJoint->setActors(&actor0, &actor1);
		mJoint->setLocalPose(physx::PxJointActorIndex::eACTOR0, localFrame0);
		mJoint->setLocalPose(physx::PxJointActorIndex::eACTOR1, localFrame1);
	}

	void FixedJoint::GetBreakForceAndTorque(float& breakForce, float& breakTorque) const
	{
		NR_CORE_VERIFY(mJoint);
		mJoint->getBreakForce(breakForce, breakTorque);
	}

	void FixedJoint::SetBreakForceAndTorque(float breakForce, float breakTorque)
	{
		NR_CORE_VERIFY(mJoint);
		mJoint->setBreakForce(breakForce, breakTorque);
	}

	bool FixedJoint::IsBroken() const
	{
		NR_CORE_VERIFY(mJoint);
		return mJoint->getConstraintFlags() & physx::PxConstraintFlag::eBROKEN;
	}

	bool FixedJoint::IsBreakable() const
	{
		NR_CORE_VERIFY(mJoint);
		float breakForce, breakTorque;
		mJoint->getBreakForce(breakForce, breakTorque);
		return breakForce == std::numeric_limits<float>::max() && breakTorque == std::numeric_limits<float>::max();
	}

	void FixedJoint::Break()
	{
		NR_CORE_VERIFY(mJoint);
		mJoint->setConstraintFlag(physx::PxConstraintFlag::eBROKEN, true);
	}

	bool FixedJoint::IsPreProcessingEnabled() const
	{
		NR_CORE_VERIFY(mJoint);
		return !(mJoint->getConstraintFlags() & physx::PxConstraintFlag::eDISABLE_PREPROCESSING);
	}

	void FixedJoint::SetPreProcessingEnabled(bool enabled)
	{
		NR_CORE_VERIFY(mJoint);
		mJoint->setConstraintFlag(physx::PxConstraintFlag::eDISABLE_PREPROCESSING, !enabled);
	}

	bool FixedJoint::IsCollisionEnabled() const
	{
		NR_CORE_VERIFY(mJoint);
		return mJoint->getConstraintFlags() & physx::PxConstraintFlag::eCOLLISION_ENABLED;
	}

	void FixedJoint::SetCollisionEnabled(bool enabled)
	{
		NR_CORE_VERIFY(mJoint);
		mJoint->setConstraintFlag(physx::PxConstraintFlag::eCOLLISION_ENABLED, enabled);
	}

	void FixedJoint::Release()
	{
		if (mJoint != nullptr)
		{
			mJoint->release();
			mJoint = nullptr;
		}
	}
}