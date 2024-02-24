#pragma once

#include "Core.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace NR
{
	using loggerPtr = std::shared_ptr<spdlog::logger>;

	class  Log
	{
	public:
		static void Initialize();

		inline static loggerPtr& GetCoreLogger() { return sCoreLogger; }
		inline static loggerPtr& GetClientLogger() { return sClientLogger; }

	private:
		static loggerPtr sCoreLogger;
		static loggerPtr sClientLogger;
	};
}

#define NR_CORE_TRACE(...) ::NR::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define NR_CORE_INFO(...)  ::NR::Log::GetCoreLogger()->info(__VA_ARGS__)
#define NR_CORE_WARN(...)  ::NR::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define NR_CORE_ERROR(...) ::NR::Log::GetCoreLogger()->error(__VA_ARGS__)

#define NR_TRACE(...)      ::NR::Log::GetClientLogger()->trace(__VA_ARGS__)
#define NR_INFO(...)       ::NR::Log::GetClientLogger()->info(__VA_ARGS__)
#define NR_WARN(...)       ::NR::Log::GetClientLogger()->warn(__VA_ARGS__)
#define NR_ERROR(...)      ::NR::Log::GetClientLogger()->error(__VA_ARGS__)
#define NR_FATAL(...)      ::NR::Log::GetClientLogger()->fatal(__VA_ARGS__)

