#pragma once

#include <string>

#include "NotRed/Core/Core.h"

namespace NR
{
	class ConsoleMessage
	{
	public:
		enum class Category
		{
			None = -1,
			Info = 1 << 0,
			Warning = 1 << 1,
			Error = 1 << 2
		};

	public:
		ConsoleMessage()
			: mMessageID(0), mMessage(""), mCount(0), mCategory(Category::None) {}
		ConsoleMessage(const std::string& message, Category category)
			: mMessageID(std::hash<std::string>()(message)), mMessage(message), mCount(1), mCategory(category) {}

		uint32_t GetMessageID() const { return mMessageID; }
		const std::string& GetMessage() const { return mMessage; }
		uint32_t GetCount() const { return mCount; }
		Category  GetCategory() const { return mCategory; }

	private:
		uint32_t mMessageID;
		std::string mMessage;
		uint32_t mCount;
		Category mCategory;

	private:
		friend class EditorConsolePanel;
	};
}