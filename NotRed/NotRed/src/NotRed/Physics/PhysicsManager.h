#pragma once

#include <PxPhysicsAPI.h>

#include "NotRed/Core/Core.h"

namespace NR
{
	class PhysicsManager
	{
	public:
		static void Init();
		static void Shutdown();

		static physx::PxSceneDesc CreateSceneDesc();
		static physx::PxScene* CreateScene(const physx::PxSceneDesc& sceneDesc);

	private:
		static physx::PxDefaultErrorCallback sPXErrorCallback;
		static physx::PxDefaultAllocator sPXAllocator;
		static physx::PxFoundation* sPXFoundation;
		static physx::PxPhysics* sPXPhysicsFactory;

	};
}