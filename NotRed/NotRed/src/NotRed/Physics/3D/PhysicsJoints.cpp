#include "nrpch.h"
#include "PhysicsJoints.h"

#include <PhysX/include/extensions/PxFixedJoint.h>

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
		physx::PxVec3 localAxis = attachActor.getGlobalPose().rotateInv(PhysicsUtils::ToPhysicsVector(globalAxis));
		physx::PxVec3 localNormal = attachActor.getGlobalPose().rotateInv(PhysicsUtils::ToPhysicsVector(globalNormal));

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
		mJoint->setBreakForce(fixedJointComponent.BreakForce, fixedJointComponent.BreakTorque);
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

	void FixedJoint::SetBreakForce(float breakForce, float breakTorque)
	{
		NR_CORE_VERIFY(mJoint);
		mJoint->setBreakForce(breakForce, breakTorque);
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