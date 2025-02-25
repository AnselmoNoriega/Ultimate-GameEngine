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
		virtual ~JointBase() {}

		virtual void SetConnectedEntity(Entity other) = 0;
		virtual void SetBreakForce(float breakForce, float breakTorque) = 0;

		Entity GetEntity() const { return mEntity; }

		virtual bool IsValid() const = 0;
		virtual void Release() = 0;

		JointType GetType() const { return mType; }

	protected:
		JointBase(Entity entity, JointType type)
			: mEntity(entity), mType(type) {}

		physx::PxTransform GetLocalFrame(physx::PxRigidActor& attachActor);

	protected:
		Entity mEntity;

	private:
		JointType mType = JointType::None;
	};

	class FixedJoint : public JointBase
	{
	public:
		FixedJoint(Entity entity);
		~FixedJoint();

		virtual void SetConnectedEntity(Entity other) override;
		virtual void SetBreakForce(float breakForce, float breakTorque) override;

		virtual bool IsValid() const override { return mJoint != nullptr; }
		virtual void Release() override;

	private:
		physx::PxFixedJoint* mJoint = nullptr;
	};
}