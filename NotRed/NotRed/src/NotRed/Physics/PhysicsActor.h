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

		float GetMass() const;
		void SetMass(float mass);

		void AddForce(const glm::vec3& force, ForceMode forceMode);
		void AddTorque(const glm::vec3& torque, ForceMode forceMode);

		glm::vec3 GetVelocity() const;
		void SetVelocity(const glm::vec3& velocity);
		glm::vec3 GetAngularVelocity() const;
		void SetAngularVelocity(const glm::vec3& velocity);

		float GetMaxVelocity() const;
		void SetMaxVelocity(float maxVelocity);
		float GetMaxAngularVelocity() const;
		void SetMaxAngularVelocity(float maxVelocity);

		void SetLinearDrag(float drag) const;
		void SetAngularDrag(float drag) const;

		glm::vec3 GetKinematicTargetPosition() const;
		glm::vec3 GetKinematicTargetRotation() const;
		void SetKinematicTarget(const glm::vec3& targetPosition, const glm::vec3& targetRotation) const;

		void SetSimulationData(uint32_t layerId);

		bool IsDynamic() const { return mRigidBodyData.BodyType == RigidBodyComponent::Type::Dynamic; }

		bool IsKinematic() const { return IsDynamic() && mRigidBodyData.IsKinematic; }
		void SetKinematic(bool isKinematic);

		bool IsGravityDisabled() const { return mRigidActor->getActorFlags().isSet(physx::PxActorFlag::eDISABLE_GRAVITY); }
		void SetGravityDisabled(bool disable);

		bool GetLockFlag(ActorLockFlag flag) const { return (uint32_t)flag & mLockFlags; }
		void ModifyLockFlag(ActorLockFlag flag, bool addFlag);

		void FixedUpdate(float fixedDeltaTime);

		void AddCollider(BoxColliderComponent& collider, Entity entity, const glm::vec3& offset = glm::vec3(0.0f));
		void AddCollider(SphereColliderComponent& collider, Entity entity, const glm::vec3& offset = glm::vec3(0.0f));
		void AddCollider(CapsuleColliderComponent& collider, Entity entity, const glm::vec3& offset = glm::vec3(0.0f));
		void AddCollider(MeshColliderComponent& collider, Entity entity, const glm::vec3& offset = glm::vec3(0.0f));

		Entity GetEntity() const { return mEntity; }
		const TransformComponent& GetTransform() const { return mEntity.GetComponent<TransformComponent>(); }

		physx::PxRigidActor& GetPhysicsActor() const { return *mRigidActor; }

		glm::vec3 GetPosition() const { return PhysicsUtils::FromPhysicsVector(mRigidActor->getGlobalPose().p); }
		void SetPosition(const glm::vec3& translation, bool autowake = true);

		glm::vec3 GetRotation() const { return glm::eulerAngles(PhysicsUtils::FromPhysicsQuat(mRigidActor->getGlobalPose().q)); }
		void SetRotation(const glm::vec3& rotation, bool autowake = true);
		void Rotate(const glm::vec3& rotation, bool autowake = true);

		void WakeUp();
		void Sleep();

	private:
		void CreateRigidActor();
		void SynchronizeTransform();

	private:
		Entity mEntity;
		RigidBodyComponent mRigidBodyData;
		uint32_t mLockFlags = 0;

		physx::PxRigidActor* mRigidActor;
		std::vector<Ref<ColliderShape>> mColliders;

	private:
		friend class PhysicsScene;
	};

}