#include "nrpch.h"
#include "PhysicsDebugger.h"

#include "NotRed/Physics/PhysicsInternal.h"

namespace NR
{
	struct PhysicsData
	{
		physx::PxPvd* Debugger;
		physx::PxPvdTransport* Transport;
	};

	static PhysicsData* sData = nullptr;

	void PhysicsDebugger::Initialize()
	{
#ifdef NR_DEBUG
		sData = new PhysicsData();

		sData->Debugger = PxCreatePvd(PhysicsInternal::GetFoundation());
		NR_CORE_ASSERT(sData->Debugger, "PxCreatePvd failed");
#endif
	}

	void PhysicsDebugger::Shutdown()
	{
#ifdef NR_DEBUG
		sData->Debugger->release();
		delete sData;
		sData = nullptr;
#endif
	}

	void PhysicsDebugger::StartDebugging(const std::string& filepath, bool networkDebugging /*= false*/)
	{
#ifdef NR_DEBUG
		StopDebugging();

		if (!networkDebugging)
		{
			sData->Transport = physx::PxDefaultPvdFileTransportCreate((filepath + ".pxd2").c_str());
			sData->Debugger->connect(*sData->Transport, physx::PxPvdInstrumentationFlag::eALL);
		}
		else
		{
			sData->Transport = physx::PxDefaultPvdSocketTransportCreate("localhost", 5425, 1000);
			sData->Debugger->connect(*sData->Transport, physx::PxPvdInstrumentationFlag::eALL);
		}
#endif
	}

	bool PhysicsDebugger::IsDebugging()
	{
#ifdef NR_DEBUG
		return sData->Debugger->isConnected();
#else
		return false;
#endif
	}

	void PhysicsDebugger::StopDebugging()
	{
#ifdef NR_DEBUG
		if (!sData->Debugger->isConnected())
		{
			return;
		}

		sData->Debugger->disconnect();
		sData->Transport->release();
#endif
	}

	physx::PxPvd* PhysicsDebugger::GetDebugger()
	{
#ifdef NR_DEBUG
		return sData->Debugger;
#else
		return nullptr;
#endif
	}
}