#include "nrpch.h"
#include "PhysicsJoints.h"

#include <PhysX/include/extensions/PxFixedJoint.h>
#include <PhysX/include/extensions/PxDistanceJoint.h>

#include "PhysicsManager.h"
#include "PhysicsInternal.h"
#include "NotRed/Script/ScriptEngine.h"

namespace NR
{
	JointBase::JointBase(Entity entity, Entity connectedEntity, JointType type)
		: mEntity(entity), mConnectedEntity(connectedEntity), mType(type)
	{
		NR_CORE_VERIFY(entity.HasComponent<RigidBodyComponent>() || connectedEntity.HasComponent<RigidBodyComponent>());
	}

	void JointBase::SetConnectedEntity(Entity other)
	{
		NR_CORE_VERIFY(mJoint);

		mConnectedEntity = other;

		ConnectedEntityChanged(other);

		auto physicsScene = Scene::GetScene(mEntity.GetSceneID())->GetPhysicsScene();

		auto& actor0 = physicsScene->GetActor(mEntity)->GetPhysicsActor();
		auto& actor1 = physicsScene->GetActor(other)->GetPhysicsActor();

		physx::PxTransform localFrame0 = GetLocalFrame(actor0);
		physx::PxTransform localFrame1 = GetLocalFrame(actor1);

		mJoint->setActors(&actor1, &actor0);
		mJoint->setLocalPose(physx::PxJointActorIndex::eACTOR0, localFrame1);
		mJoint->setLocalPose(physx::PxJointActorIndex::eACTOR1, localFrame0);
	}

	void JointBase::GetBreakForceAndTorque(float& breakForce, float& breakTorque) const
	{
		NR_CORE_VERIFY(mJoint);
		mJoint->getBreakForce(breakForce, breakTorque);
	}

	void JointBase::SetBreakForceAndTorque(float breakForce, float breakTorque)
	{
		NR_CORE_VERIFY(mJoint);
		mJoint->setBreakForce(breakForce, breakTorque);
	}

	bool JointBase::IsBroken() const
	{
		NR_CORE_VERIFY(mJoint);
		return mJoint->getConstraintFlags() & physx::PxConstraintFlag::eBROKEN;
	}

	void JointBase::Break()
	{
		NR_CORE_VERIFY(mJoint);
		mJoint->setConstraintFlag(physx::PxConstraintFlag::eBROKEN, true);
	}

	bool JointBase::IsPreProcessingEnabled() const
	{
		NR_CORE_VERIFY(mJoint);
		return !(mJoint->getConstraintFlags() & physx::PxConstraintFlag::eDISABLE_PREPROCESSING);
	}

	void JointBase::SetPreProcessingEnabled(bool enabled)
	{
		NR_CORE_VERIFY(mJoint);
		mJoint->setConstraintFlag(physx::PxConstraintFlag::eDISABLE_PREPROCESSING, !enabled);
	}

	bool JointBase::IsCollisionEnabled() const
	{
		NR_CORE_VERIFY(mJoint);
		return mJoint->getConstraintFlags() & physx::PxConstraintFlag::eCOLLISION_ENABLED;
	}

	void JointBase::SetCollisionEnabled(bool enabled)
	{
		NR_CORE_VERIFY(mJoint);
		mJoint->setConstraintFlag(physx::PxConstraintFlag::eCOLLISION_ENABLED, enabled);
	}

	void JointBase::PostSimulation()
	{
		physx::PxVec3 linear(0.0f), angular(0.0f);
		mJoint->getConstraint()->getForce(linear, angular);
		mLastReportedLinearForce = PhysicsUtils::FromPhysicsVector(linear);
		mLastReportedAngularForce = PhysicsUtils::FromPhysicsVector(angular);
	}

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

	void JointBase::Release()
	{
		if (mJoint != nullptr)
		{
			mJoint->release();
			mJoint = nullptr;
		}
	}

	FixedJoint::FixedJoint(Entity entity, Entity connectedEntity)
		: JointBase(entity, connectedEntity, JointType::Fixed)
	{
		auto physicsScene = Scene::GetScene(entity.GetSceneID())->GetPhysicsScene();

		const auto& fixedJointComponent = entity.GetComponent<FixedJointComponent>();
		NR_CORE_VERIFY(connectedEntity.GetID() == fixedJointComponent.ConnectedEntity);

		auto& actor0 = physicsScene->GetActor(entity)->GetPhysicsActor();
		auto& actor1 = physicsScene->GetActor(connectedEntity)->GetPhysicsActor();

		physx::PxTransform localFrame0 = GetLocalFrame(actor0);
		physx::PxTransform localFrame1 = GetLocalFrame(actor1);

		mJoint = physx::PxFixedJointCreate(PhysicsInternal::GetPhysicsSDK(), &actor1, localFrame1, &actor0, localFrame0);
		NR_CORE_ASSERT(mJoint, "Failed to create FixedJoint!");

		if (fixedJointComponent.IsBreakable)
			mJoint->setBreakForce(fixedJointComponent.BreakForce, fixedJointComponent.BreakTorque);
		else
			mJoint->setBreakForce(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

		mJoint->setConstraintFlag(physx::PxConstraintFlag::eCOLLISION_ENABLED, fixedJointComponent.EnableCollision);
		mJoint->setConstraintFlag(physx::PxConstraintFlag::eDISABLE_PREPROCESSING, !fixedJointComponent.EnablePreProcessing);
		mJoint->userData = this;
	}

	FixedJoint::~FixedJoint()
	{
	}

	void FixedJoint::ConnectedEntityChanged(Entity other)
	{
		auto& fixedJointComponent = mEntity.GetComponent<FixedJointComponent>();
		fixedJointComponent.ConnectedEntity = mConnectedEntity.GetID();
	}

	bool FixedJoint::IsBreakable() const
	{
		NR_CORE_VERIFY(mJoint);
		const auto& fixedJointComponent = mEntity.GetComponent<FixedJointComponent>();
		return fixedJointComponent.IsBreakable;
	}
}