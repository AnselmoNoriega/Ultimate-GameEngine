#pragma once

#include "Event.h"
#include "NotRed/Core/KeyCodes.h"

#include <sstream>

namespace NR
{
	class NOT_RED_API KeyEvent : public Event
	{
	public:
		inline int GetKeyCode() const { return mKeyCode; }

		EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
	protected:
		KeyEvent(int keycode)
			: mKeyCode(keycode) {}

		int mKeyCode;
	};

	class NOT_RED_API KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent(int keycode, int repeatCount)
			: KeyEvent(keycode), mRepeatCount(repeatCount) {}

		inline int GetRepeatCount() const { return mRepeatCount; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyPressedEvent: " << mKeyCode << " (" << mRepeatCount << " repeats)";
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyPressed)

	private:
		int mRepeatCount;
	};

	class NOT_RED_API KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(int keycode)
			: KeyEvent(keycode) {}


		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyReleasedEvent: " << mKeyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyReleased)
	};

	class NOT_RED_API KeyTypedEvent : public KeyEvent
	{
	public:
		KeyTypedEvent(int keycode)
			: KeyEvent(keycode) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyTypedEvent: " << mKeyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyTyped)
	};
}