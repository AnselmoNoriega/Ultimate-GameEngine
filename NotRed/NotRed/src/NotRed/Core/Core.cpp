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