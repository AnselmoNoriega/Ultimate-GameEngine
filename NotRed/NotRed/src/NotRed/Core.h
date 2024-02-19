#pragma once

#ifdef NR_PLATFORM_WINDOWS
	#ifdef  NR_BUILD_DLL
		#define NR_API __declspec(dllexport)
	#else
		#define NR_API __declspec(dllimport)
	#endif
#else
	#error This Engine only supports Windows.
#endif

#ifdef NR_ENABLE_ASSERTS
	#define NR_ASSERT(x, ...) { if(!(x)) { NR_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define NR_CORE_ASSERT(x, ...) { if(!(x)) { NR_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }

	#define NR_VERIFY(x, ...) { if(!(x)) { NR_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define NR_CORE_VERIFY(x, ...) { if(!(x)) { NR_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define NR_ASSERT(x, ...)
	#define NR_CORE_ASSERT(x, ...)

	#define NR_VERIFY(x, ...) x
	#define NR_CORE_VERIFY(x, ...) x
#endif