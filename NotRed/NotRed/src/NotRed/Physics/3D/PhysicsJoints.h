#pragma once

#include "NotRed/Scene/Entity.h"
#include "PhysicsActor.h"

namespace physx {
	class PxRigidActor;
	class PxFixedJoint;
}

namespace NR
{
	enum class JointType
	{
		None, Fixed, Hinge, Spring
	};

	class JointBase : public RefCounted
	{
	public:
		virtual ~JointBase()
		{
			Release();
		}

		void SetConnectedEntity(Entity other);

		void GetBreakForceAndTorque(float& breakForce, float& breakTorque) const;
		void SetBreakForceAndTorque(float breakForce, float breakTorque);
		bool IsBroken() const;
		void Break();
		virtual bool IsBreakable() const = 0;

		bool IsPreProcessingEnabled() const;
		void SetPreProcessingEnabled(bool enabled);

		bool IsCollisionEnabled() const;
		void SetCollisionEnabled(bool enabled);

		const glm::vec3& GetLastReportedLinearForce() const { return mLastReportedLinearForce; }
		const glm::vec3& GetLastReportedAngularForce() const { return mLastReportedAngularForce; }

		Entity GetEntity() const { return mEntity; }
		Entity GetConnectedEntity() const { return mConnectedEntity; }

		bool IsValid() const { return mJoint != nullptr; }
		void Release();

		virtual const char* GetDebugName() const = 0;

		JointType GetType() const { return mType; }

	private:
		void PostSimulation();

	protected:
		JointBase(Entity entity, Entity connectedEntity, JointType type);

		physx::PxTransform GetLocalFrame(physx::PxRigidActor& attachActor);

		virtual void ConnectedEntityChanged(Entity other) = 0;

	protected:
		Entity mEntity, mConnectedEntity;
		physx::PxJoint* mJoint = nullptr;

	private:
		JointType mType = JointType::None;
		glm::vec3 mLastReportedLinearForce = glm::vec3(0.0f);
		glm::vec3 mLastReportedAngularForce = glm::vec3(0.0f);

	private:
		friend class PhysicsScene;
	};

	class FixedJoint : public JointBase
	{
	public:
		FixedJoint(Entity entity, Entity connectedEntity);
		~FixedJoint() override;

		bool IsBreakable() const override;

		const char* GetDebugName() const override { return "FixedJoint"; }

	protected:
		void ConnectedEntityChanged(Entity other) override;
	};
}