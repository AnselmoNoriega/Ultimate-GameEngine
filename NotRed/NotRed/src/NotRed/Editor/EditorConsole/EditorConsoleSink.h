#pragma once

#include <mutex>

#include "NotRed/Editor/EditorConsolePanel.h"

#include "spdlog/sinks/base_sink.h"

namespace NR
{
	class EditorConsoleSink : public spdlog::sinks::base_sink<std::mutex>
	{
	public:
		explicit EditorConsoleSink(uint32_t bufferCapacity)
			: mMessageBufferCapacity(bufferCapacity), mMessageBuffer(bufferCapacity) {}
		EditorConsoleSink(const EditorConsoleSink& other) = delete;

		~EditorConsoleSink() override = default;

		EditorConsoleSink& operator=(const EditorConsoleSink& other) = delete;

	protected:
		void sink_it_(const spdlog::details::log_msg& msg) override
		{
			spdlog::memory_buf_t formatted;
			spdlog::sinks::base_sink<std::mutex>::formatter_->format(msg, formatted);
			mMessageBuffer[mMessageCount++] = ConsoleMessage(fmt::to_string(formatted), GetMessageCategory(msg.level));

			if (mMessageCount == mMessageBufferCapacity)
			{
				flush_();
			}
		}

		void flush_() override
		{
			for (const auto& message : mMessageBuffer)
			{
				if (message.GetCategory() == ConsoleMessage::Category::None)
				{
					continue;
				}

				EditorConsolePanel::PushMessage(message);
			}
			mMessageCount = 0;
		}

	private:
		static ConsoleMessage::Category GetMessageCategory(spdlog::level::level_enum level)
		{
			switch (level)
			{
			case spdlog::level::trace:
			case spdlog::level::debug:
			case spdlog::level::info:
			{
				return ConsoleMessage::Category::Info;
			}
			case spdlog::level::warn:
			{
				return ConsoleMessage::Category::Warning;
			}
			case spdlog::level::err:
			case spdlog::level::critical:
			{
				return ConsoleMessage::Category::Error;
			}
			default:
			{
				NR_CORE_ASSERT("Invalid Message Category!");
				return ConsoleMessage::Category::None;
			}
			}
		}

	private:
		uint32_t mMessageBufferCapacity;
		std::vector<ConsoleMessage> mMessageBuffer;
		uint32_t mMessageCount = 0;
	};
}