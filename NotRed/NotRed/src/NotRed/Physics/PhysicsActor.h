#pragma once

#include "NotRed/Scene/Entity.h"
#include "NotRed/Physics/PhysicsManager.h"

namespace physx
{
	class PxRigidActor;
	class PxShape;
	class PxMaterial;
}

namespace NR
{
	class PhysicsActor : public RefCounted
	{
	public:
		PhysicsActor(Entity entity);
		~PhysicsActor();

		glm::vec3 GetPosition();
		glm::quat GetRotation();
		void Rotate(const glm::vec3& rotation);

		float GetMass() const;
		void SetMass(float mass);

		void AddForce(const glm::vec3& force, ForceMode forceMode);
		void AddTorque(const glm::vec3& torque, ForceMode forceMode);

		glm::vec3 GetVelocity() const;
		void SetVelocity(const glm::vec3& velocity);
		glm::vec3 GetAngularVelocity() const;
		void SetAngularVelocity(const glm::vec3& velocity);

		void SetDrag(float drag) const;
		void SetAngularDrag(float drag) const;

		void SetLayer(uint32_t layerId);

		bool IsDynamic() const { return mRigidBody.BodyType == RigidBodyComponent::Type::Dynamic; }

		Entity& GetEntity() { return mEntity; }

	private:
		void Initialize();
		void Spawn();
		void Update(float FixedDeltaTime);

		void SynchronizeTransform();
		void AddCollisionShape(physx::PxShape* shape);

	private:
		Entity mEntity;
		RigidBodyComponent& mRigidBody;

		physx::PxRigidActor* mActorInternal;
		std::unordered_map<int, std::vector<physx::PxShape*>> mShapes;

	private:
		friend class PhysicsManager;
		friend class PhysicsWrappers;
	};

}