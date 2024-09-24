#include "nrpch.h"
#include "PhysicsInternal.h"

#include "CookingFactory.h"

#include "Debug/PhysicsDebugger.h"

namespace NR
{
	struct PhysicsData
	{
		physx::PxFoundation* Foundation;
		physx::PxDefaultCpuDispatcher* CPUDispatcher;
		physx::PxPhysics* PhysicsSDK;

		PhysicsAllocator Allocator;
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
			NR_CORE_INFO("[PhysX]: {0}: {1} at {2} ({3})", errorMessage, message, file, line);
			break;
		case physx::PxErrorCode::eDEBUG_WARNING:
		case physx::PxErrorCode::ePERF_WARNING:
			NR_CORE_WARN("[PhysX]: {0}: {1} at {2} ({3})", errorMessage, message, file, line);
			break;
		case physx::PxErrorCode::eINVALID_PARAMETER:
		case physx::PxErrorCode::eINVALID_OPERATION:
		case physx::PxErrorCode::eOUT_OF_MEMORY:
		case physx::PxErrorCode::eINTERNAL_ERROR:
			NR_CORE_ERROR("[PhysX]: {0}: {1} at {2} ({3})", errorMessage, message, file, line);
			break;
		case physx::PxErrorCode::eABORT:
		case physx::PxErrorCode::eMASK_ALL:
			NR_CORE_FATAL("[PhysX]: {0}: {1} at {2} ({3})", errorMessage, message, file, line);
			NR_CORE_ASSERT(false);
			break;
		}
	}

	PhysicsAllocator::~PhysicsAllocator()
	{
	}

	void* PhysicsAllocator::allocate(size_t size, const char* typeName, const char* filename, int line)
	{
		void* ptr = physx::platformAlignedAlloc(size);
		NR_CORE_ASSERT((reinterpret_cast<size_t>(ptr) & 15) == 0);
		return ptr;
	}

	void PhysicsAllocator::deallocate(void* ptr)
	{
		physx::platformAlignedFree(ptr);
	}

	void PhysicsInternal::Initialize()
	{
		NR_CORE_ASSERT(!sPhysicsData, "Trying to initialize the PhysX SDK multiple times!");

		sPhysicsData = new PhysicsData();

		sPhysicsData->Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, sPhysicsData->Allocator, sPhysicsData->ErrorCallback);
		NR_CORE_ASSERT(sPhysicsData->Foundation, "PxCreateFoundation failed.");

		physx::PxTolerancesScale scale = physx::PxTolerancesScale();
		scale.length = 1.0f;
		scale.speed = 9.81f;

		PhysicsDebugger::Initialize();

		static bool s_TrackMemoryAllocations = true; // Disable for release builds
		sPhysicsData->PhysicsSDK = PxCreatePhysics(PX_PHYSICS_VERSION, *sPhysicsData->Foundation, scale, s_TrackMemoryAllocations, PhysicsDebugger::GetDebugger());
		NR_CORE_ASSERT(sPhysicsData->PhysicsSDK, "PxCreatePhysics failed.");

		NR_CORE_ASSERT(PxInitExtensions(*sPhysicsData->PhysicsSDK, PhysicsDebugger::GetDebugger()), "Failed to initialize PhysX Extensions.");

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

	PhysicsAllocator& PhysicsInternal::GetAllocator() 
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

		if ((filterData0.word0 & filterData1.word1) || (filterData1.word0 & filterData0.word1))
		{
			pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;
			pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_LOST;
			return physx::PxFilterFlag::eDEFAULT;
		}

		return physx::PxFilterFlag::eSUPPRESS;
	}

	physx::PxBroadPhaseType::Enum PhysicsInternal::ToPhysicsBroadphaseType(BroadphaseType type)
	{
		switch (type)
		{
		case BroadphaseType::SweepAndPrune: return physx::PxBroadPhaseType::eSAP;
		case BroadphaseType::MultiBoxPrune: return physx::PxBroadPhaseType::eMBP;
		case BroadphaseType::AutomaticBoxPrune: return physx::PxBroadPhaseType::eABP;
		}

		return physx::PxBroadPhaseType::eABP;
	}

	physx::PxFrictionType::Enum PhysicsInternal::ToPhysicsFrictionType(FrictionType type)
	{
		switch (type)
		{
		case FrictionType::Patch:			return physx::PxFrictionType::ePATCH;
		case FrictionType::OneDirectional:	return physx::PxFrictionType::eONE_DIRECTIONAL;
		case FrictionType::TwoDirectional:	return physx::PxFrictionType::eTWO_DIRECTIONAL;
		}

		return physx::PxFrictionType::ePATCH;
	}
}