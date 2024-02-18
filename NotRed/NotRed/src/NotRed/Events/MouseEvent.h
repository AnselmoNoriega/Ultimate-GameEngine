#pragma once

#include "Event.h"

namespace NR
{
    class NR_API MouseMovedEvent : public Event
    {
    public:
        MouseMovedEvent(float x, float y)
            : mMouseX(x), mMouseY(y) {}

        inline float GetX() const { return mMouseX; }
        inline float GetY() const { return mMouseY; }


        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "MouseMovedEvent: " << mMouseX << ", " << mMouseY;
            return ss.str();
        }

        EVENT_CLASS_TYPE(MouseMoved)
        EVENT_CLASS_CATEGORY(EVENTCATEGORYMOUSE | EVENTCATEGORYINPUT)

    private:
        float mMouseX, mMouseY;
    };

    class NR_API MouseScrolledEvent : public Event
    {
    public:
        MouseScrolledEvent(float xOffset, float yOffset)
            : mXOffset(xOffset), mYOffset(yOffset) {}

        inline float GetX_Offset() const { return mXOffset; }
        inline float GetY_Offset() const { return mYOffset; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "MouseScrolledEvent: " << mXOffset << ", " << mYOffset;
            return ss.str();
        }

        EVENT_CLASS_TYPE(MouseScrolled)
        EVENT_CLASS_CATEGORY(EVENTCATEGORYMOUSE | EVENTCATEGORYINPUT)

    private:
        float mXOffset, mYOffset;
    };

    class NR_API MouseButtonEvent : public Event
    {
    public:

        inline int GetMouseButton() const { return mButton; }

        EVENT_CLASS_CATEGORY(EVENTCATEGORYMOUSE | EVENTCATEGORYINPUT)

    protected:
        MouseButtonEvent(int button)
            : mButton(button) {}

        int mButton;
    };

    class NR_API MouseButtonPressedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonPressedEvent(int button)
            : MouseButtonEvent(button) {}

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "MouseButtonPressedEvent: " << mButton;
            return ss.str();
        }

        EVENT_CLASS_TYPE(MouseButtonPressed)
    };

    class NR_API MouseButtonReleasedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonReleasedEvent(int button)
            : MouseButtonEvent(button) {}

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "MouseButtonReleasedEvent: " << mButton;
            return ss.str();
        }

        EVENT_CLASS_TYPE(MouseButtonReleased)
    };
}