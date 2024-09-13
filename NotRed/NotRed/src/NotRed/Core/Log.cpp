#include "nrpch.h"
#include "Log.h"

#include <filesystem>

#include "spdlog/sinks/stdout_sinks.h"
#include <spdlog/sinks/basic_file_sink.h>
#include "spdlog/sinks/stdout_color_sinks.h"

namespace NR
{
	std::shared_ptr<spdlog::logger> Log::sCoreLogger;
	std::shared_ptr<spdlog::logger> Log::sClientLogger;

	void Log::Init()
	{
		std::string logsDirectory = "logs";
		if (!std::filesystem::exists(logsDirectory))
		{
			std::filesystem::create_directories(logsDirectory);
		}

		std::vector<spdlog::sink_ptr> engineSinks =
		{
			std::make_shared<spdlog::sinks::stdout_sink_mt>(),
			std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/NOTRED.log", true)
		};

		std::vector<spdlog::sink_ptr> appSinks =
		{
			std::make_shared<spdlog::sinks::stdout_sink_mt>(),
			std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/APP.log", true)
		};

		sCoreLogger = std::make_shared<spdlog::logger>("NOT-RED", engineSinks.begin(), engineSinks.end());
		sCoreLogger->set_pattern("\033[1;34m[%Y-%m-%d %H:%M:%S.%e]\033[0m %^[%l]%$ \033[4;31m%n\033[0m: %v");
		sCoreLogger->set_level(spdlog::level::trace);

		sClientLogger = std::make_shared<spdlog::logger>("APP", appSinks.begin(), appSinks.end());
		sClientLogger->set_pattern("\033[1;34m[%Y-%m-%d %H:%M:%S.%e]\033[0m %^[%l]%$ \033[4;36m%n\033[0m: %v");
		sClientLogger->set_level(spdlog::level::trace);
	}

	void Log::Shutdown()
	{
		sClientLogger.reset();
		sCoreLogger.reset();
		spdlog::drop_all();
	}
}
