#include "nrpch.h"
#include "PhysicsShapes.h"

#include <filesystem>

#include <glm/gtc/type_ptr.hpp>

#include "NotRed/Asset/AssetManager.h"
#include "PhysicsInternal.h"
#include "PhysicsActor.h"
#include "PhysicsSystem.h"

#include "NotRed/Math/Math.h"
#include "NotRed/Util/FileSystem.h"

//#include "NotRed/Project/Project.h"
#include "NotRed/Debug/Profiler.h"

namespace NR
{
	void ColliderShape::SetMaterial(AssetHandle material)
	{
		Ref<PhysicsMaterial> mat = AssetManager::GetAsset<PhysicsMaterial>(material);
		if (!mat)
		{
			mat = Ref<PhysicsMaterial>::Create(0.6f, 0.6f, 0.0f);
		}

		if (mMaterial != nullptr)
		{
			mMaterial->release();
		}

		mMaterial = PhysicsInternal::GetPhysicsSDK().createMaterial(mat->StaticFriction, mat->DynamicFriction, mat->Bounciness);
	}

	BoxColliderShape::BoxColliderShape(BoxColliderComponent& component, const PhysicsActor& actor, Entity entity, const glm::vec3& offset)
		: ColliderShape(ColliderType::Box), mComponent(component)
	{
		SetMaterial(mComponent.Material);

		glm::vec3 colliderSize = entity.Transform().Scale * mComponent.Size;
		physx::PxBoxGeometry geometry = physx::PxBoxGeometry(colliderSize.x / 2.0f, colliderSize.y / 2.0f, colliderSize.z / 2.0f);
		mShape = physx::PxRigidActorExt::createExclusiveShape(actor.GetPhysicsActor(), geometry, *mMaterial);
		mShape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !mComponent.IsTrigger);
		mShape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, mComponent.IsTrigger);
		mShape->setLocalPose(PhysicsUtils::ToPhysicsTransform(offset + mComponent.Offset, glm::vec3(0.0f)));
		mShape->userData = this;
	}

	void BoxColliderShape::SetOffset(const glm::vec3& offset)
	{
		mShape->setLocalPose(PhysicsUtils::ToPhysicsTransform(offset, glm::vec3(0.0f)));
		mComponent.Offset = offset;
	}

	void BoxColliderShape::SetTrigger(bool isTrigger)
	{
		mShape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !isTrigger);
		mShape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, isTrigger);
		mComponent.IsTrigger = isTrigger;
	}

	void BoxColliderShape::DetachFromActor(physx::PxRigidActor* actor)
	{
		actor->detachShape(*mShape);
	}

	void BoxColliderShape::SetFilterData(const physx::PxFilterData& filterData)
	{
		mShape->setSimulationFilterData(filterData);
	}

	SphereColliderShape::SphereColliderShape(SphereColliderComponent& component, const PhysicsActor& actor, Entity entity, const glm::vec3& offset)
		: ColliderShape(ColliderType::Sphere), mComponent(component)
	{
		SetMaterial(mComponent.Material);

		auto& actorScale = entity.Transform().Scale;
		float largestComponent = glm::max(actorScale.x, glm::max(actorScale.y, actorScale.z));

		physx::PxSphereGeometry geometry = physx::PxSphereGeometry(largestComponent * mComponent.Radius);
		mShape = physx::PxRigidActorExt::createExclusiveShape(actor.GetPhysicsActor(), geometry, *mMaterial);
		mShape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !mComponent.IsTrigger);
		mShape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, mComponent.IsTrigger);
		mShape->setLocalPose(PhysicsUtils::ToPhysicsTransform(offset + mComponent.Offset, glm::vec3(0.0f)));
		mShape->userData = this;
	}

	void SphereColliderShape::SetOffset(const glm::vec3& offset)
	{
		mShape->setLocalPose(PhysicsUtils::ToPhysicsTransform(offset, glm::vec3(0.0f)));
		mComponent.Offset = offset;
	}

	void SphereColliderShape::SetTrigger(bool isTrigger)
	{
		mShape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !isTrigger);
		mShape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, isTrigger);
		mComponent.IsTrigger = isTrigger;
	}

	void SphereColliderShape::DetachFromActor(physx::PxRigidActor* actor)
	{
		actor->detachShape(*mShape);
	}

	void SphereColliderShape::SetFilterData(const physx::PxFilterData& filterData)
	{
		mShape->setSimulationFilterData(filterData);
	}

	CapsuleColliderShape::CapsuleColliderShape(CapsuleColliderComponent& component, const PhysicsActor& actor, Entity entity, const glm::vec3& offset)
		: ColliderShape(ColliderType::Capsule), mComponent(component)
	{
		SetMaterial(mComponent.Material);

		auto& actorScale = entity.Transform().Scale;
		float radiusScale = glm::max(actorScale.x, actorScale.z);

		physx::PxCapsuleGeometry geometry = physx::PxCapsuleGeometry(mComponent.Radius * radiusScale, (mComponent.Height / 2.0f) * actorScale.y);
		mShape = physx::PxRigidActorExt::createExclusiveShape(actor.GetPhysicsActor(), geometry, *mMaterial);
		mShape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !mComponent.IsTrigger);
		mShape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, mComponent.IsTrigger);
		mShape->setLocalPose(PhysicsUtils::ToPhysicsTransform(offset + mComponent.Offset, glm::vec3(0.0f, 0.0f, physx::PxHalfPi)));
		mShape->userData = this;
	}

	void CapsuleColliderShape::SetOffset(const glm::vec3& offset)
	{
		mShape->setLocalPose(PhysicsUtils::ToPhysicsTransform(offset, glm::vec3(0.0f)));
		mComponent.Offset = offset;
	}

	void CapsuleColliderShape::SetTrigger(bool isTrigger)
	{
		mShape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !isTrigger);
		mShape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, isTrigger);
		mComponent.IsTrigger = isTrigger;
	}

	void CapsuleColliderShape::DetachFromActor(physx::PxRigidActor* actor)
	{
		actor->detachShape(*mShape);
	}

	void CapsuleColliderShape::SetFilterData(const physx::PxFilterData& filterData)
	{
		mShape->setSimulationFilterData(filterData);
	}

	ConvexMeshShape::ConvexMeshShape(MeshColliderComponent& component, const PhysicsActor& actor, Entity entity, const glm::vec3& offset)
		: ColliderShape(ColliderType::ConvexMesh), mComponent(component)
	{
		NR_PROFILE_FUNC();

		NR_CORE_ASSERT(AssetManager::GetMetadata(component.CollisionMesh).Type == AssetType::Mesh);

		SetMaterial(mComponent.Material);

		const MeshColliderData& meshData = PhysicsSystem::GetMeshCache().GetMeshData(component.CollisionMesh);
		NR_CORE_ASSERT(meshData.Submeshes.size() > component.SubmeshIndex);

		const SubmeshColliderData& submesh = meshData.Submeshes[component.SubmeshIndex];
		glm::vec3 submeshTranslation, submeshRotation, submeshScale;
		Math::DecomposeTransform(submesh.Transform, submeshTranslation, submeshRotation, submeshScale);

		glm::vec3 scale = entity.Transform().Scale;
		std::swap(scale.y, scale.z);

		physx::PxDefaultMemoryInputData input(submesh.ColliderData.As<physx::PxU8>(), submesh.ColliderData.Size);
		physx::PxConvexMesh* convexMesh = PhysicsInternal::GetPhysicsSDK().createConvexMesh(input);
		physx::PxConvexMeshGeometry convexGeometry = physx::PxConvexMeshGeometry(convexMesh, physx::PxMeshScale(PhysicsUtils::ToPhysicsVector(submeshScale * scale)));
		convexGeometry.meshFlags = physx::PxConvexMeshGeometryFlag::eTIGHT_BOUNDS;

		physx::PxShape* shape = PhysicsInternal::GetPhysicsSDK().createShape(convexGeometry, *mMaterial, true);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !mComponent.IsTrigger);
		shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, mComponent.IsTrigger);
		shape->setLocalPose(PhysicsUtils::ToPhysicsTransform(submeshTranslation, submeshRotation));

		actor.GetPhysicsActor().attachShape(*shape);

		mShapes.push_back(shape);

		shape->release();
		convexMesh->release();
	}

	void ConvexMeshShape::SetTrigger(bool isTrigger)
	{
		for (auto shape : mShapes)
		{
			shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !isTrigger);
			shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, isTrigger);
		}

		mComponent.IsTrigger = isTrigger;
	}

	void ConvexMeshShape::DetachFromActor(physx::PxRigidActor* actor)
	{
		for (auto shape : mShapes)
		{
			actor->detachShape(*shape);
		}

		mShapes.clear();
	}

	void ConvexMeshShape::SetFilterData(const physx::PxFilterData& filterData)
	{
		for (auto shape : mShapes)
		{
			shape->setSimulationFilterData(filterData);
		}
	}

	TriangleMeshShape::TriangleMeshShape(MeshColliderComponent& component, const PhysicsActor& actor, Entity entity, const glm::vec3& offset)
		: ColliderShape(ColliderType::TriangleMesh), mComponent(component)
	{
		NR_PROFILE_FUNC();

		NR_CORE_ASSERT(AssetManager::GetMetadata(component.CollisionMesh).Type == AssetType::StaticMesh);

		SetMaterial(mComponent.Material);

		auto collisionMesh = AssetManager::GetAsset<Mesh>(component.CollisionMesh);

		const MeshColliderData& meshData = PhysicsSystem::GetMeshCache().GetMeshData(component.CollisionMesh);

		for (size_t i = 0; i < meshData.Submeshes.size(); ++i)
		{
			const SubmeshColliderData& submeshData = meshData.Submeshes[i];

			physx::PxDefaultMemoryInputData input(submeshData.ColliderData.As<physx::PxU8>(), submeshData.ColliderData.Size);
			physx::PxTriangleMesh* trimesh = PhysicsInternal::GetPhysicsSDK().createTriangleMesh(input);
			physx::PxTriangleMeshGeometry triangleGeometry = physx::PxTriangleMeshGeometry(trimesh, physx::PxMeshScale(PhysicsUtils::ToPhysicsVector(entity.Transform().Scale)));

			physx::PxShape* shape = PhysicsInternal::GetPhysicsSDK().createShape(triangleGeometry, *mMaterial, true);
			shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !mComponent.IsTrigger);
			shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, mComponent.IsTrigger);
			shape->setLocalPose(PhysicsUtils::ToPhysicsTransform(submeshData.Transform));

			actor.GetPhysicsActor().attachShape(*shape);

			mShapes.push_back(shape);

			shape->release();
			trimesh->release();
		}
	}

	void TriangleMeshShape::SetTrigger(bool isTrigger)
	{
		for (auto shape : mShapes)
		{
			shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !isTrigger);
			shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, isTrigger);
		}
		mComponent.IsTrigger = isTrigger;
	}

	void TriangleMeshShape::SetFilterData(const physx::PxFilterData& filterData)
	{
		for (auto shape : mShapes)
		{
			shape->setSimulationFilterData(filterData);
		}
	}

	void TriangleMeshShape::DetachFromActor(physx::PxRigidActor* actor)
	{
		for (auto shape : mShapes)
		{
			actor->detachShape(*shape);
		}
		mShapes.clear();
	}
}