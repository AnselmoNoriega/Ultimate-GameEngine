#pragma once

#include "Event.h"

#include <sstream>

namespace NR
{
    class NOT_RED_API WindowCloseEvent : public Event
    {
    public:
        WindowCloseEvent() = default;

        EVENT_CLASS_TYPE(WindowClose)
            EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class NOT_RED_API WindowResizeEvent : public Event
    {
    public:
        WindowResizeEvent(unsigned int width, unsigned int height)
            : mWidth(width), mHeight(height) {}

        inline unsigned int GetWidth() const { return mWidth; }
        inline unsigned int GetHeight() const { return mHeight; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "WindowResizeEvent: " << mWidth << ", " << mHeight;
            return ss.str();
        }

        EVENT_CLASS_TYPE(WindowResize)
            EVENT_CLASS_CATEGORY(EventCategoryApplication)

    private:
        unsigned int mWidth, mHeight;
    };

    class NOT_RED_API AppTickEvent : public Event
    {
    public:
        AppTickEvent() = default;

        EVENT_CLASS_TYPE(AppTick)
            EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class NOT_RED_API AppUpdateEvent : public Event
    {
    public:
        AppUpdateEvent() = default;

        EVENT_CLASS_TYPE(AppUpdate)
            EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class NOT_RED_API AppRenderEvent : public Event
    {
    public:
        AppRenderEvent() = default;

        EVENT_CLASS_TYPE(AppRender)
            EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };
}