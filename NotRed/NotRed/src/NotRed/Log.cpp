#include "nrpch.h"
#include "Log.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace NR
{
	loggerPtr Log::sCoreLogger;
	loggerPtr Log::sClientLogger;

	void Log::Initialize()
	{
		spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] %^[%l]%$ %n: %v");
		sCoreLogger = spdlog::stdout_color_mt("NOT-RED");
		sCoreLogger->set_level(spdlog::level::trace);

		sClientLogger = spdlog::stdout_color_mt("APP");
		sClientLogger->set_level(spdlog::level::trace);
	}
}
