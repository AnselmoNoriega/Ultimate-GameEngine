#include "nrpch.h"
#include "PhysicsManager.h"

namespace NR
{
	physx::PxDefaultErrorCallback PhysicsManager::sPXErrorCallback;
	physx::PxDefaultAllocator PhysicsManager::sPXAllocator;
	physx::PxFoundation* PhysicsManager::sPXFoundation;
	physx::PxPhysics* PhysicsManager::sPXPhysicsFactory;

	void PhysicsManager::Init()
	{
		NR_CORE_ASSERT(!sPXFoundation, "PhysicsManager::Init shouldn't be called more than once!");

		sPXFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, sPXAllocator, sPXErrorCallback);
		NR_CORE_ASSERT(sPXFoundation, "PxCreateFoundation Failed!");

		sPXPhysicsFactory = PxCreatePhysics(PX_PHYSICS_VERSION, *sPXFoundation, physx::PxTolerancesScale(), true);
		NR_CORE_ASSERT(sPXPhysicsFactory, "PxCreatePhysics Failed!");
	}

	void PhysicsManager::Shutdown()
	{
		sPXPhysicsFactory->release();
		sPXFoundation->release();
	}

	physx::PxSceneDesc PhysicsManager::CreateSceneDesc()
	{
		return physx::PxSceneDesc(sPXPhysicsFactory->getTolerancesScale());
	}

	physx::PxScene* PhysicsManager::CreateScene(const physx::PxSceneDesc& sceneDesc)
	{
		return sPXPhysicsFactory->createScene(sceneDesc);
	}
}