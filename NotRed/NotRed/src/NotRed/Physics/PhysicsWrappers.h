#pragma once

#include "NotRed/Physics/PhysicsUtil.h"
#include "NotRed/Scene/Components.h"

namespace NR
{
	struct SceneParams;

	class PhysicsWrappers
	{
	public:
		static physx::PxScene* CreateScene(const SceneParams& sceneParams);
		static physx::PxRigidActor* CreateActor(const RigidBodyComponent& rigidbody, const glm::mat4& transform);
		static void SetCollisionFilters(const physx::PxRigidActor& actor, uint32_t actorGroup, uint32_t filters);

		static void AddBoxCollider(physx::PxRigidActor& actor, const physx::PxMaterial& material, const BoxColliderComponent& collider);
		static void AddSphereCollider(physx::PxRigidActor& actor, const physx::PxMaterial& material, const SphereColliderComponent& collider);
		static void AddCapsuleCollider(physx::PxRigidActor& actor, const physx::PxMaterial& material, const CapsuleColliderComponent& collider);
		static void AddMeshCollider(physx::PxRigidActor& actor, const physx::PxMaterial& material, const MeshColliderComponent& collider);

		static physx::PxMaterial* CreateMaterial(const PhysicsMaterialComponent& material);

	private:
		static void Initialize();
		static void Shutdown();

		static void ConnectVisualDebugger();
		static void DisconnectVisualDebugger();

	private:
		friend class PhysicsManager;
	};
}