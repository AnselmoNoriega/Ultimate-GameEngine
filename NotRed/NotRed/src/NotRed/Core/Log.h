#pragma once

#include <glm/glm.hpp>
#include <spdlog/fmt/fmt.h>
#include <filesystem>

#include "NotRed/Core/Core.h"

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace NR
{
	class Log
	{
	public:
		static void Init();
		static void Shutdown();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return sCoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return sClientLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetEditorConsoleLogger() { return sEditorConsoleLogger; }

	private:
		static std::shared_ptr<spdlog::logger> sCoreLogger;
		static std::shared_ptr<spdlog::logger> sClientLogger;
		static std::shared_ptr<spdlog::logger> sEditorConsoleLogger;
	};

}

template<typename OStream>
OStream& operator<<(OStream& os, const glm::vec3& vec)
{
	return os << '(' << vec.x << ", " << vec.y << ", " << vec.z << ')';
}

template<typename OStream>
OStream& operator<<(OStream& os, const glm::vec4& vec)
{
	return os << '(' << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << ')';
}

template <>
struct fmt::formatter<std::filesystem::path> : fmt::formatter<std::string>
{
	template <typename FormatContext>
	auto format(const std::filesystem::path& path, FormatContext& ctx)
	{
		return fmt::formatter<std::string>::format(path.string(), ctx);
	}
};

// Core Logging Macros
#define NR_CORE_TRACE(...)	NR::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define NR_CORE_INFO(...)	NR::Log::GetCoreLogger()->info(__VA_ARGS__)
#define NR_CORE_WARN(...)	NR::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define NR_CORE_ERROR(...)	NR::Log::GetCoreLogger()->error(__VA_ARGS__)
#define NR_CORE_FATAL(...)	NR::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client Logging Macros
#define NR_TRACE(...)	NR::Log::GetClientLogger()->trace(__VA_ARGS__)
#define NR_INFO(...)	NR::Log::GetClientLogger()->info(__VA_ARGS__)
#define NR_WARN(...)	NR::Log::GetClientLogger()->warn(__VA_ARGS__)
#define NR_ERROR(...)	NR::Log::GetClientLogger()->error(__VA_ARGS__)
#define NR_FATAL(...)	NR::Log::GetClientLogger()->critical(__VA_ARGS__)

// Editor Console Logging Macros
#define NR_CONSOLE_LOG_TRACE(...)	NR::Log::GetEditorConsoleLogger()->trace(__VA_ARGS__)
#define NR_CONSOLE_LOG_INFO(...)	NR::Log::GetEditorConsoleLogger()->info(__VA_ARGS__)
#define NR_CONSOLE_LOG_WARN(...)	NR::Log::GetEditorConsoleLogger()->warn(__VA_ARGS__)
#define NR_CONSOLE_LOG_ERROR(...)	NR::Log::GetEditorConsoleLogger()->error(__VA_ARGS__)
#define NR_CONSOLE_LOG_FATAL(...)	NR::Log::GetEditorConsoleLogger()->critical(__VA_ARGS__)