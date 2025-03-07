#include "nrpch.h"
#include "PhysicsInternal.h"

#include "CookingFactory.h"

#include "NotRed/Math/Math.h"

#include "Debug/PhysicsDebugger.h"

namespace NR
{
	struct PhysicsData
	{
		physx::PxFoundation* Foundation;
		physx::PxDefaultCpuDispatcher* CPUDispatcher;
		physx::PxPhysics* PhysicsSDK;

		physx::PxDefaultAllocator Allocator;
		PhysicsErrorCallback ErrorCallback;
	};

	static PhysicsData* sPhysicsData;

	void PhysicsErrorCallback::reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line)
	{
		const char* errorMessage = NULL;

		switch (code)
		{
		case physx::PxErrorCode::eNO_ERROR:				errorMessage = "No Error"; break;
		case physx::PxErrorCode::eDEBUG_INFO:			errorMessage = "Info"; break;
		case physx::PxErrorCode::eDEBUG_WARNING:		errorMessage = "Warning"; break;
		case physx::PxErrorCode::eINVALID_PARAMETER:	errorMessage = "Invalid Parameter"; break;
		case physx::PxErrorCode::eINVALID_OPERATION:	errorMessage = "Invalid Operation"; break;
		case physx::PxErrorCode::eOUT_OF_MEMORY:		errorMessage = "Out Of Memory"; break;
		case physx::PxErrorCode::eINTERNAL_ERROR:		errorMessage = "Internal Error"; break;
		case physx::PxErrorCode::eABORT:				errorMessage = "Abort"; break;
		case physx::PxErrorCode::ePERF_WARNING:			errorMessage = "Performance Warning"; break;
		case physx::PxErrorCode::eMASK_ALL:				errorMessage = "Unknown Error"; break;
		}

		switch (code)
		{
		case physx::PxErrorCode::eNO_ERROR:
		case physx::PxErrorCode::eDEBUG_INFO:
		{
			NR_CORE_INFO("[PhysX]: {0}: {1} at {2} ({3})", errorMessage, message, file, line);
			break;
		}
		case physx::PxErrorCode::eDEBUG_WARNING:
		case physx::PxErrorCode::ePERF_WARNING:
		{
			NR_CORE_WARN("[PhysX]: {0}: {1} at {2} ({3})", errorMessage, message, file, line);
			break;
		}
		case physx::PxErrorCode::eINVALID_PARAMETER:
		case physx::PxErrorCode::eINVALID_OPERATION:
		case physx::PxErrorCode::eOUT_OF_MEMORY:
		case physx::PxErrorCode::eINTERNAL_ERROR:
		{
			NR_CORE_ERROR("[PhysX]: {0}: {1} at {2} ({3})", errorMessage, message, file, line);
			break;
		}
		case physx::PxErrorCode::eABORT:
		case physx::PxErrorCode::eMASK_ALL:
		{
			NR_CORE_FATAL("[PhysX]: {0}: {1} at {2} ({3})", errorMessage, message, file, line);
			NR_CORE_ASSERT(false);
			break;
		}
		}
	}

	void PhysicsInternal::Initialize()
	{
		NR_CORE_ASSERT(!sPhysicsData, "Trying to initialize the PhysX SDK multiple times!");

		sPhysicsData = new PhysicsData();

		sPhysicsData->Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, sPhysicsData->Allocator, sPhysicsData->ErrorCallback);
		NR_CORE_ASSERT(sPhysicsData->Foundation, "PxCreateFoundation failed.");

		physx::PxTolerancesScale scale = physx::PxTolerancesScale();
		scale.length = 1.0f;
		scale.speed = 100.0f;

		PhysicsDebugger::Initialize();

#ifdef NR_DEBUG
		static bool sTrackMemoryAllocations = true;
#else
		static bool sTrackMemoryAllocations = false;
#endif

		sPhysicsData->PhysicsSDK = PxCreatePhysics(PX_PHYSICS_VERSION, *sPhysicsData->Foundation, scale, sTrackMemoryAllocations, PhysicsDebugger::GetDebugger());
		NR_CORE_ASSERT(sPhysicsData->PhysicsSDK, "PxCreatePhysics failed.");

		bool extentionsLoaded = PxInitExtensions(*sPhysicsData->PhysicsSDK, PhysicsDebugger::GetDebugger());
		NR_CORE_ASSERT(extentionsLoaded, "Failed to initialize Physics Extensions.");

		sPhysicsData->CPUDispatcher = physx::PxDefaultCpuDispatcherCreate(1);

		CookingFactory::Initialize();
	}

	void PhysicsInternal::Shutdown()
	{
		CookingFactory::Shutdown();

		sPhysicsData->CPUDispatcher->release();
		sPhysicsData->CPUDispatcher = nullptr;

		PxCloseExtensions();

		PhysicsDebugger::StopDebugging();

		sPhysicsData->PhysicsSDK->release();
		sPhysicsData->PhysicsSDK = nullptr;

		PhysicsDebugger::Shutdown();

		sPhysicsData->Foundation->release();
		sPhysicsData->Foundation = nullptr;

		delete sPhysicsData;
		sPhysicsData = nullptr;
	}

	physx::PxFoundation& PhysicsInternal::GetFoundation() 
	{ 
		return *sPhysicsData->Foundation; 
	}

	physx::PxPhysics& PhysicsInternal::GetPhysicsSDK() 
	{ 
		return *sPhysicsData->PhysicsSDK; 
	
	}
	physx::PxCpuDispatcher* PhysicsInternal::GetCPUDispatcher() 
	{ 
		return sPhysicsData->CPUDispatcher; 
	}

	physx::PxDefaultAllocator& PhysicsInternal::GetAllocator()
	{ 
		return sPhysicsData->Allocator; 
	}

	physx::PxFilterFlags PhysicsInternal::FilterShader(physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0, physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1, physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize)
	{
		if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1))
		{
			pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
			return physx::PxFilterFlag::eDEFAULT;
		}

		pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;

		if (filterData0.word2 == (uint32_t)RigidBodyComponent::CollisionDetectionType::Continuous || filterData1.word2 == (uint32_t)RigidBodyComponent::CollisionDetectionType::Continuous)
		{
			pairFlags |= physx::PxPairFlag::eDETECT_DISCRETE_CONTACT;
			pairFlags |= physx::PxPairFlag::eDETECT_CCD_CONTACT;
		}

		if ((filterData0.word0 & filterData1.word1) || (filterData1.word0 & filterData0.word1))
		{
			pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;
			pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_LOST;
			return physx::PxFilterFlag::eDEFAULT;
		}

		return physx::PxFilterFlag::eSUPPRESS;
	}
}