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

#define NR_EXPAND_VARGS(x) x

#define NR_BIND_EVENT_FN(fn) std::bind(&##fn, this, std::placeholders::_1)

#include "Assert.h"

// Pointer wrappers
namespace NR
{
	template<typename T>
	using Scope = std::unique_ptr<T>;

	template<typename T>
	using Ref = std::shared_ptr<T>;

	using byte = unsigned char;
}