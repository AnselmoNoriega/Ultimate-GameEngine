#pragma once

#include "PhysicsSettings.h"

#include <PxPhysicsAPI.h>

namespace NR
{
	class PhysicsErrorCallback : public physx::PxErrorCallback
	{
	public:
		void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) override;
	};

	class PhysicsInternal
	{
	public:
		static void Initialize();
		static void Shutdown();

		static physx::PxFoundation& GetFoundation();
		static physx::PxPhysics& GetPhysicsSDK();
		static physx::PxCpuDispatcher* GetCPUDispatcher();
		static physx::PxDefaultAllocator& GetAllocator();

		static physx::PxFilterFlags FilterShader(
			physx::PxFilterObjectAttributes attributes0, 
			physx::PxFilterData filterData0, 
			physx::PxFilterObjectAttributes attributes1,
			physx::PxFilterData filterData1, 
			physx::PxPairFlags& pairFlags, 
			const void* constantBlock, 
			physx::PxU32 constantBlockSize);

		static physx::PxBroadPhaseType::Enum ToPhysicsBroadphaseType(BroadphaseType type);
		static physx::PxFrictionType::Enum ToPhysicsFrictionType(FrictionType type);
	};
}