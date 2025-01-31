#pragma once

#include <PhysX/include/PxPhysicsAPI.h>

namespace NR
{
	class PhysicsDebugger
	{
	public:
		static void Initialize();
		static void Shutdown();

		static void StartDebugging(const std::string& filepath, bool networkDebugging = false);
		static bool IsDebugging();
		static void StopDebugging();

		static physx::PxPvd* GetDebugger();
	};
}