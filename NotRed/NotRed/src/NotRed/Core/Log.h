#pragma once

#include "NotRed/Core/Core.h"

#include "spdlog/spdlog.h"
#include "spdlog/fmt/bundled/ostream.h"

namespace NR
{
	class NOT_RED_API Log
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return sCoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return sClientLogger; }
	private:
		static std::shared_ptr<spdlog::logger> sCoreLogger;
		static std::shared_ptr<spdlog::logger> sClientLogger;
	};

}

// Core Logging Macros
#define NR_CORE_TRACE(...)	hz::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define NR_CORE_INFO(...)	hz::Log::GetCoreLogger()->info(__VA_ARGS__)
#define NR_CORE_WARN(...)	hz::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define NR_CORE_ERROR(...)	hz::Log::GetCoreLogger()->error(__VA_ARGS__)
#define NR_CORE_FATAL(...)	hz::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client Logging Macros
#define NR_TRACE(...)	hz::Log::GetClientLogger()->trace(__VA_ARGS__)
#define NR_INFO(...)	hz::Log::GetClientLogger()->info(__VA_ARGS__)
#define NR_WARN(...)	hz::Log::GetClientLogger()->warn(__VA_ARGS__)
#define NR_ERROR(...)	hz::Log::GetClientLogger()->error(__VA_ARGS__)
#define NR_FATAL(...)	hz::Log::GetClientLogger()->critical(__VA_ARGS__)