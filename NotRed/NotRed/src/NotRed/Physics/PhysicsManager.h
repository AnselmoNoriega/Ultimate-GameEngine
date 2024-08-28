#pragma once

#include <PxPhysicsAPI.h>

#include "NotRed/Scene/Components.h"

namespace NR
{
	enum class ForceMode : uint16_t
	{
		Force,
		Impulse,
		VelocityChange,
		Acceleration
	};

	enum class FilterGroup : uint32_t
	{
		Static = 1 << 0,
		Dynamic = 1 << 1,
		Kinematic = 1 << 2,
		All = Static | Dynamic | Kinematic
	};

	class PhysicsManager
	{
	public:
		static void Init();
		static void Shutdown();

		static physx::PxSceneDesc CreateSceneDesc();
		static physx::PxScene* CreateScene(const physx::PxSceneDesc& sceneDesc);
		static physx::PxRigidActor* AddActor(physx::PxScene* scene, const RigidBodyComponent& rigidbody, const glm::mat4& transform);
		static physx::PxMaterial* CreateMaterial(float staticFriction, float dynamicFriction, float restitution);
		static physx::PxConvexMesh* CreateMeshCollider(const Ref<Mesh>& mesh);

		static physx::PxTransform CreatePose(const glm::mat4& transform);

		static void SetCollisionFilters(physx::PxRigidActor* actor, uint32_t filterGroup, uint32_t filterMask);

	private:
		static physx::PxDefaultErrorCallback sPXErrorCallback;
		static physx::PxDefaultAllocator sPXAllocator;
		static physx::PxFoundation* sPXFoundation;
		static physx::PxPhysics* sPXPhysicsFactory;
		static physx::PxPvd* sPXPvd;
	};
}