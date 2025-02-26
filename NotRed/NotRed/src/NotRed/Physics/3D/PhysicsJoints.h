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

		virtual void GetBreakForceAndTorque(float& breakForce, float& breakTorque) const = 0;
		virtual void SetBreakForceAndTorque(float breakForce, float breakTorque) = 0;
		virtual bool IsBroken() const = 0;
		virtual bool IsBreakable() const = 0;
		virtual void Break() = 0;

		virtual bool IsPreProcessingEnabled() const = 0;
		virtual void SetPreProcessingEnabled(bool enabled) = 0;

		virtual bool IsCollisionEnabled() const = 0;
		virtual void SetCollisionEnabled(bool enabled) = 0;

		Entity GetEntity() const { return mEntity; }

		virtual bool IsValid() const = 0;
		virtual void Release() = 0;

		virtual const char* GetDebugName() const = 0;

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

		void SetConnectedEntity(Entity other) override;

		void GetBreakForceAndTorque(float& breakForce, float& breakTorque) const override;
		void SetBreakForceAndTorque(float breakForce, float breakTorque) override;
		bool IsBroken() const override;
		bool IsBreakable() const override;
		void Break() override;

		bool IsPreProcessingEnabled() const override;
		void SetPreProcessingEnabled(bool enabled) override;

		bool IsCollisionEnabled() const override;
		void SetCollisionEnabled(bool enabled) override;

		const char* GetDebugName() const override { return "FixedJoint"; }

		bool IsValid() const override { return mJoint != nullptr; }
		void Release() override;

	private:
		physx::PxFixedJoint* mJoint = nullptr;
	};
}