#pragma once

#include "PhysicsUtils.h"
#include "PhysicsShapes.h"
#include "NotRed/Scene/Entity.h"

namespace NR
{
	class PhysicsActor : public RefCounted
	{
	public:
		PhysicsActor(Entity entity);
		~PhysicsActor();

		void Rotate(const glm::vec3& rotation);

		float GetMass() const;
		void SetMass(float mass);

		void AddForce(const glm::vec3& force, ForceMode forceMode);
		void AddTorque(const glm::vec3& torque, ForceMode forceMode);

		const glm::vec3& GetVelocity() const;
		void SetVelocity(const glm::vec3& velocity);
		const glm::vec3& GetAngularVelocity() const;
		void SetAngularVelocity(const glm::vec3& velocity);

		void SetLinearDrag(float drag) const;
		void SetAngularDrag(float drag) const;

		void SetLayer(uint32_t layerId);

		bool IsDynamic() const { return mRigidBodyData.BodyType == RigidBodyComponent::Type::Dynamic; }

		bool IsKinematic() const { return IsDynamic() && mRigidBodyData.IsKinematic; }
		void SetKinematic(bool isKinematic);

		bool IsGravityDisabled() const { return mRigidActor->getActorFlags().isSet(physx::PxActorFlag::eDISABLE_GRAVITY); }
		void SetGravityDisabled(bool disable);

		bool GetLockFlag(ActorLockFlag flag) const { return (uint32_t)flag & mLockFlags; }
		void ModifyLockFlag(ActorLockFlag flag, bool addFlag);

		void AddCollider(BoxColliderComponent& collider);
		void AddCollider(SphereColliderComponent& collider);
		void AddCollider(CapsuleColliderComponent& collider);
		void AddCollider(MeshColliderComponent& collider);

		Entity GetEntity() const { return mEntity; }
		const TransformComponent& GetTransform() const { return mEntity.GetComponent<TransformComponent>(); }

		physx::PxRigidActor& GetPhysicsActor() const { return *mRigidActor; }

		const glm::vec3& GetPosition() const { return PhysicsUtils::FromPhysicsVector(mRigidActor->getGlobalPose().p); }
		const glm::vec3& GetRotation() const { return glm::eulerAngles(PhysicsUtils::FromPhysicsQuat(mRigidActor->getGlobalPose().q)); }

	private:
		void CreateRigidActor();
		void SynchronizeTransform();

	private:
		Entity mEntity;
		RigidBodyComponent& mRigidBodyData;
		uint32_t mLockFlags = 0;

		physx::PxRigidActor* mRigidActor;
		std::vector<Ref<ColliderShape>> mColliders;

	private:
		friend class PhysicsScene;
	};

}