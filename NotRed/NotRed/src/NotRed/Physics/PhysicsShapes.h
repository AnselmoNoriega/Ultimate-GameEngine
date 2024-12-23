#pragma once

#include "CookingFactory.h"

#include "PhysicsUtils.h"
#include "NotRed/Scene/Entity.h"

namespace NR
{
	enum class ColliderType
	{
		Box, Sphere, Capsule, ConvexMesh, TriangleMesh
	};

	class ColliderShape : public RefCounted
	{
	protected:
		ColliderShape(ColliderType type)
			: mType(type), mMaterial(nullptr) {}

	public:
		virtual ~ColliderShape() {}

		void Release()
		{
			mMaterial->release();
		}

		void SetMaterial(const Ref<PhysicsMaterial>& material);

		virtual const glm::vec3& GetOffset() const = 0;
		virtual void SetOffset(const glm::vec3& offset) = 0;

		virtual bool IsTrigger() const = 0;
		virtual void SetTrigger(bool isTrigger) = 0;

		virtual void DetachFromActor(physx::PxRigidActor* actor) = 0;

		virtual void SetFilterData(const physx::PxFilterData& filterData) = 0;

		physx::PxMaterial& GetMaterial() const { return *mMaterial; }

		bool IsValid() const { return mMaterial != nullptr; }

	protected:
		ColliderType mType;

		physx::PxMaterial* mMaterial;
	};

	class PhysicsActor;

	class BoxColliderShape : public ColliderShape
	{
	public:
		BoxColliderShape(BoxColliderComponent& component, const PhysicsActor& actor, Entity entity, const glm::vec3& offset = glm::vec3(0.0f));
		~BoxColliderShape() override = default;

		void DetachFromActor(physx::PxRigidActor* actor) override;

		void SetOffset(const glm::vec3& offset) override;
		void SetTrigger(bool isTrigger) override;

		const glm::vec3& GetOffset() const override { return mComponent.Offset; }
		bool IsTrigger() const override { return mComponent.IsTrigger; }

		void SetFilterData(const physx::PxFilterData& filterData) override;

	private:
		BoxColliderComponent& mComponent;
		physx::PxShape* mShape;
	};

	class SphereColliderShape : public ColliderShape
	{
	public:
		SphereColliderShape(SphereColliderComponent& component, const PhysicsActor& actor, Entity entity, const glm::vec3& offset = glm::vec3(0.0f));
		~SphereColliderShape() override = default;

		void DetachFromActor(physx::PxRigidActor* actor) override;

		void SetOffset(const glm::vec3& offset) override;
		void SetTrigger(bool isTrigger) override;

		const glm::vec3& GetOffset() const override { return mComponent.Offset; }
		bool IsTrigger() const override { return mComponent.IsTrigger; }

		void SetFilterData(const physx::PxFilterData& filterData) override;

	private:
		SphereColliderComponent& mComponent;
		physx::PxShape* mShape;
	};

	class CapsuleColliderShape : public ColliderShape
	{
	public:
		CapsuleColliderShape(CapsuleColliderComponent& component, const PhysicsActor& actor, Entity entity, const glm::vec3& offset = glm::vec3(0.0f));
		~CapsuleColliderShape() override = default;

		void DetachFromActor(physx::PxRigidActor* actor) override;

		void SetOffset(const glm::vec3& offset) override;
		void SetTrigger(bool isTrigger) override;

		const glm::vec3& GetOffset() const override { return mComponent.Offset; }
		bool IsTrigger() const override { return mComponent.IsTrigger; }

		void SetFilterData(const physx::PxFilterData& filterData) override;

	private:
		CapsuleColliderComponent& mComponent;
		physx::PxShape* mShape;
	};

	class ConvexMeshShape : public ColliderShape
	{
	public:
		ConvexMeshShape(MeshColliderComponent& component, const PhysicsActor& actor, Entity entity, const glm::vec3& offset = glm::vec3(0.0f));
		~ConvexMeshShape() override = default;

		void SetOffset(const glm::vec3& offset) override {};
		void SetTrigger(bool isTrigger) override;

		const glm::vec3& GetOffset() const override { return glm::vec3(0.0f); }
		bool IsTrigger() const override { return mComponent.IsTrigger; }
		
		void SetFilterData(const physx::PxFilterData& filterData) override;
		void DetachFromActor(physx::PxRigidActor* actor) override;

	private:
		MeshColliderComponent& mComponent;
		std::vector<physx::PxShape*> mShapes;
	};

	class TriangleMeshShape : public ColliderShape
	{
	public:
		TriangleMeshShape(MeshColliderComponent& component, const PhysicsActor& actor, Entity entity, const glm::vec3& offset = glm::vec3(0.0f));
		~TriangleMeshShape() override = default;

		void DetachFromActor(physx::PxRigidActor* actor) override;

		void SetOffset(const glm::vec3& offset) override {};
		void SetTrigger(bool isTrigger) override;

		const glm::vec3& GetOffset() const override { return glm::vec3(0.0f); }
		bool IsTrigger() const override { return mComponent.IsTrigger; }

		void SetFilterData(const physx::PxFilterData& filterData) override;

	private:
		MeshColliderComponent& mComponent;
		std::vector<physx::PxShape*> mShapes;
	};
}