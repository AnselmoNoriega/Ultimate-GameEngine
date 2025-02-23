#include "nrpch.h"
#include "PhysicsDebugger.h"

#include "NotRed/Physics/3D/PhysicsInternal.h"

namespace NR
{
	struct PhysicsData
	{
		physx::PxPvd* Debugger;
		physx::PxPvdTransport* Transport;
	};

	static PhysicsData* sData = nullptr;

#ifdef NR_DEBUG
	void PhysicsDebugger::Initialize()
	{
		sData = new PhysicsData();

		sData->Debugger = PxCreatePvd(PhysicsInternal::GetFoundation());
		NR_CORE_ASSERT(sData->Debugger, "PxCreatePvd failed");
	}

	void PhysicsDebugger::Shutdown()
	{
		sData->Debugger->release();
		delete sData;
		sData = nullptr;
	}

	void PhysicsDebugger::StartDebugging(const std::string& filepath, bool networkDebugging /*= false*/)
	{
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
	}

	bool PhysicsDebugger::IsDebugging()
	{
		return sData->Debugger->isConnected();
	}

	void PhysicsDebugger::StopDebugging()
	{
		if (!sData->Debugger->isConnected())
		{
			return;
		}

		sData->Debugger->disconnect();
		sData->Transport->release();
	}

	physx::PxPvd* PhysicsDebugger::GetDebugger()
	{
		return sData->Debugger;
	}
#else
	void PhysicsDebugger::Initialize() {}
	void PhysicsDebugger::Shutdown() {}
	void PhysicsDebugger::StartDebugging(const std::string& filepath, bool networkDebugging /*= false*/) {}
	bool PhysicsDebugger::IsDebugging() { return false; }
	void PhysicsDebugger::StopDebugging() {}

	physx::PxPvd* PhysicsDebugger::GetDebugger() { return nullptr; }
#endif
}