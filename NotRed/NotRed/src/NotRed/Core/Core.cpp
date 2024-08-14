#include "nrpch.h"
#include "Core.h"

#include "Log.h"

#define NOT_RED_BUILD_ID "v0.1a"

namespace NR
{
	void InitializeCore()
	{
		Log::Init();

		NR_CORE_TRACE("NotRed Engine {}", NOT_RED_BUILD_ID);
		NR_CORE_TRACE("Initializing...");
	}

	void ShutdownCore()
	{
		NR_CORE_TRACE("Shutting down...");
	}

}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		NR::InitializeCore();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		NR::ShutdownCore();
		break;
	}
	return TRUE;
}