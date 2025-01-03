#include "nrpch.h"
#include "Log.h"

#include <ozz/base/log.h>

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include "NotRed/Editor/EditorConsole/EditorConsoleSink.h"

namespace NR
{
	std::shared_ptr<spdlog::logger> Log::sCoreLogger;
	std::shared_ptr<spdlog::logger> Log::sClientLogger;
	std::shared_ptr<spdlog::logger> Log::sEditorConsoleLogger;

	void Log::Init()
	{
		// turn off ozz logging
		ozz::log::SetLevel(ozz::log::kSilent);

		// Create "logs" directory if doesn't exist
		std::string logsDirectory = "logs";
		if (!std::filesystem::exists(logsDirectory))
		{
			std::filesystem::create_directories(logsDirectory);
		}

		std::vector<spdlog::sink_ptr> engineSinks =
		{ 
			std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
			std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/NOT-RED.log", true)
		};

		std::vector<spdlog::sink_ptr> appSinks =
		{ 
			std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
			std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/APP.log", true),
			std::make_shared<EditorConsoleSink>(1)
		};

		std::vector<spdlog::sink_ptr> editorConsoleSinks =
		{
			std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
			std::make_shared<EditorConsoleSink>(1)
		};

		engineSinks[0]->set_pattern("%^[%T] %n: %v%$");
		engineSinks[1]->set_pattern("[%T] [%l] %n: %v");
		appSinks[0]->set_pattern("%^[%T] %n: %v%$");
		appSinks[1]->set_pattern("[%T] [%l] %n: %v");
		appSinks[2]->set_pattern("%^[%T] %n: %v%$");
		editorConsoleSinks[0]->set_pattern("%^[%T] %n: %v%$");
		editorConsoleSinks[1]->set_pattern("%^[%T] %n: %v%$");

		sCoreLogger = std::make_shared<spdlog::logger>("NOT-RED", engineSinks.begin(), engineSinks.end());
		sCoreLogger->set_pattern("\033[1;34m[%Y-%m-%d %H:%M:%S.%e]\033[0m %^[%l]%$ \033[4;31m%n\033[0m: %v");
		sCoreLogger->set_level(spdlog::level::trace);

		sClientLogger = std::make_shared<spdlog::logger>("APP", appSinks.begin(), appSinks.end());
		sClientLogger->set_pattern("\033[1;34m[%Y-%m-%d %H:%M:%S.%e]\033[0m %^[%l]%$ \033[4;36m%n\033[0m: %v");
		sClientLogger->set_level(spdlog::level::trace);

		sEditorConsoleLogger = std::make_shared<spdlog::logger>("Console", editorConsoleSinks.begin(), editorConsoleSinks.end());
		sEditorConsoleLogger->set_pattern("\033[1;34m[%Y-%m-%d %H:%M:%S.%e]\033[0m %^[%l]%$ \033[1;36m%n\033[0m: %v");
		sEditorConsoleLogger->set_level(spdlog::level::trace);
	}

	void Log::Shutdown()
	{
		sEditorConsoleLogger.reset();
		sClientLogger.reset();
		sCoreLogger.reset();
		spdlog::drop_all();
	}
}
