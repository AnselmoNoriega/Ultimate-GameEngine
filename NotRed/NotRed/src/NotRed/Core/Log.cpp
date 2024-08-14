#include "nrpch.h"
#include "Log.h"

#include "spdlog/sinks/stdout_color_sinks.h"

namespace NR
{
	std::shared_ptr<spdlog::logger> Log::sCoreLogger;
	std::shared_ptr<spdlog::logger> Log::sClientLogger;

	void Log::Init()
	{
		sCoreLogger = spdlog::stdout_color_mt("NOT-RED");
		sCoreLogger->set_pattern("\033[1;34m[%Y-%m-%d %H:%M:%S.%e]\033[0m %^[%l]%$ \033[4;31m%n\033[0m: %v");
		sCoreLogger->set_level(spdlog::level::trace);

		sClientLogger = spdlog::stdout_color_mt("APP");
		sClientLogger->set_pattern("\033[1;34m[%Y-%m-%d %H:%M:%S.%e]\033[0m %^[%l]%$ \033[4;36m%n\033[0m: %v");
		sClientLogger->set_level(spdlog::level::trace);
	}
}
