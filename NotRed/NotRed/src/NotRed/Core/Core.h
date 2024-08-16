#pragma once

#include <memory>

#include "Log.h"

namespace NR
{
	void InitializeCore();
	void ShutdownCore();
}

#ifdef NR_DEBUG
	#define NR_ENABLE_ASSERTS
#endif

#ifdef NR_ENABLE_ASSERTS
	#define NR_ASSERT_NO_MESSAGE(condition) { if(!(condition)) { NR_ERROR("Assertion Failed!"); __debugbreak(); } }
	#define NR_ASSERT_MESSAGE(condition, ...) { if(!(condition)) { NR_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }

	#define NR_ASSERT_RESOLVE(arg1, arg2, macro, ...) macro

	#define NR_ASSERT(...) NR_ASSERT_RESOLVE(__VA_ARGS__, NR_ASSERT_MESSAGE, NR_ASSERT_NO_MESSAGE)(__VA_ARGS__)
	#define NR_CORE_ASSERT(...) NR_ASSERT_RESOLVE(__VA_ARGS__, NR_ASSERT_MESSAGE, NR_ASSERT_NO_MESSAGE)(__VA_ARGS__)
#else
	#define NR_ASSERT(...)
	#define NR_CORE_ASSERT(...)
#endif

#define NR_BIND_EVENT_FN(fn) std::bind(&##fn, this, std::placeholders::_1)

// Pointer wrappers
namespace NR
{
	template<typename T>
	using Scope = std::unique_ptr<T>;

	template<typename T>
	using Ref = std::shared_ptr<T>;

	using byte = unsigned char;
}