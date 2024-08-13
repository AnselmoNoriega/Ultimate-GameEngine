#pragma once

#include "Event.h"

namespace NR
{
	class NOT_RED_API MouseMovedEvent : public Event
	{
	public:
		MouseMovedEvent(float x, float y, float dx, float dy)
			: mMouseX(x), mMouseY(y), mMouseDX(x), mMouseDY(dy) {}

		inline float GetX() const { return mMouseX; }
		inline float GetY() const { return mMouseY; }

		EVENT_CLASS_TYPE(MouseMoved)
			EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	private:
		float mMouseX, mMouseY;
		float mMouseDX, mMouseDY;
	};

	class NOT_RED_API MouseButtonEvent : public Event
	{
	public:
		inline int GetMouseButton() const { return mButton; }

		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	protected:
		MouseButtonEvent(int button)
			: mButton(button) {}

		int mButton;
	};

	class NOT_RED_API MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonPressedEvent(int button, int repeatCount)
			: MouseButtonEvent(button), mRepeatCount(repeatCount) {}

		inline int GetRepeatCount() const { return mRepeatCount; }

		EVENT_CLASS_TYPE(MouseButtonPressed)

	private:
		int mRepeatCount;
	};

	class NOT_RED_API MouseButtonReleasedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonReleasedEvent(int button)
			: MouseButtonEvent(button) {}

		EVENT_CLASS_TYPE(MouseButtonReleased)
	};

}