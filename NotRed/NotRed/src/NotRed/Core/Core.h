#pragma once

#include  <memory>

#ifdef NR_PLATFORM_WINDOWS

#else
	#error This Engine only supports Windows.
#endif

#ifdef NR_DEBUG
	#define NR_ENABLE_ASSERTS
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

#define NR_BIND_EVENT_FN(fn) [this](auto&&... args)-> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace NR
{
	template<typename T>
	using Scope = std::unique_ptr<T>;

	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using Ref = std::shared_ptr<T>;

	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}
}