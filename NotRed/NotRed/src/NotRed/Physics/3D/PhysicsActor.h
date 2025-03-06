#pragma once

#include "PhysicsActorBase.h"
#include "PhysicsShapes.h"

namespace NR
{
	enum class EFalloffMode { Constant, Linear };

	class PhysicsActor : public PhysicsActorBase
	{
	public:
		PhysicsActor(Entity entity);
		~PhysicsActor();

		float GetMass() const;
		void SetMass(float mass);

		float GetInverseMass() const;

		glm::mat4 GetCenterOfMass() const;
		glm::mat4 GetLocalCenterOfMass() const;

		void AddForce(const glm::vec3& force, ForceMode forceMode);
		void AddTorque(const glm::vec3& torque, ForceMode forceMode);
		void AddRadialImpulse(const glm::vec3& origin, float radius, float strength, EFalloffMode falloff = EFalloffMode::Constant, bool velocityChange = false);

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
		float GetLinearDrag() const;
		float GetAngularDrag() const;

		glm::vec3 GetKinematicTargetPosition() const;
		glm::vec3 GetKinematicTargetRotation() const;
		void SetKinematicTarget(const glm::vec3& targetPosition, const glm::vec3& targetRotation) const;

		void SetSimulationData(uint32_t layerId) override;

		bool IsDynamic() const { return mRigidBodyData.BodyType == RigidBodyComponent::Type::Dynamic; }

		bool IsKinematic() const { return IsDynamic() && mRigidBodyData.IsKinematic; }
		void SetKinematic(bool isKinematic);

		bool IsGravityDisabled() const { return mRigidActor->getActorFlags().isSet(physx::PxActorFlag::eDISABLE_GRAVITY); }
		void SetGravityDisabled(bool disable);

		bool IsLockFlagSet(ActorLockFlag flag) const { return (uint32_t)flag & mLockFlags; }
		void ModifyLockFlag(ActorLockFlag flag, bool addFlag);
		uint32_t GetLockFlags() const { return mLockFlags; }

		void AddCollider(BoxColliderComponent& collider, Entity entity, const glm::vec3& offset = glm::vec3(0.0f));
		void AddCollider(SphereColliderComponent& collider, Entity entity, const glm::vec3& offset = glm::vec3(0.0f));
		void AddCollider(CapsuleColliderComponent& collider, Entity entity, const glm::vec3& offset = glm::vec3(0.0f));
		void AddCollider(MeshColliderComponent& collider, Entity entity, const glm::vec3& offset = glm::vec3(0.0f));

		const TransformComponent& GetTransform() const { return mEntity.GetComponent<TransformComponent>(); }

		physx::PxRigidActor& GetPhysicsActor() const { return *mRigidActor; }

		const std::vector<Ref<ColliderShape>>& GetCollisionShapes() const { return mColliders; }

		glm::vec3 GetPosition() const { return PhysicsUtils::FromPhysicsVector(mRigidActor->getGlobalPose().p); }
		void SetPosition(const glm::vec3& translation, bool autowake = true);

		glm::vec3 GetRotation() const { return glm::eulerAngles(PhysicsUtils::FromPhysicsQuat(mRigidActor->getGlobalPose().q)); }
		void SetRotation(const glm::vec3& rotation, bool autowake = true);
		void Rotate(const glm::vec3& rotation, bool autowake = true);

		void WakeUp();
		void Sleep();
		bool IsSleeping() const;

	private:
		void CreateRigidActor();
		void FixedUpdate(float fixedDeltaTime);
		void SynchronizeTransform() override;

	private:
		RigidBodyComponent mRigidBodyData;
		uint32_t mLockFlags = 0;

		physx::PxRigidActor* mRigidActor = nullptr;
		std::vector<Ref<ColliderShape>> mColliders;

	private:
		friend class PhysicsScene;
	};

}