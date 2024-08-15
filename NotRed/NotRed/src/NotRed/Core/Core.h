#pragma once

#include <memory>

namespace NR
{
	void InitializeCore();
	void ShutdownCore();
}

#ifdef NR_DEBUG
	#define NR_ENABLE_ASSERTS
#endif

#ifdef NR_ENABLE_ASSERTS
	#define NR_ASSERT(x, ...) { if(!(x)) { NR_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define NR_CORE_ASSERT(x, ...) { if(!(x)) { NR_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define NR_ASSERT(x, ...)
	#define NR_CORE_ASSERT(x, ...)
#endif

#define NR_BIND_EVENT_FN(fn) std::bind(&##fn, this, std::placeholders::_1)