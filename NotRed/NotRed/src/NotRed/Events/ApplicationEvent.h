#pragma once

#include "Event.h"

#include <sstream>

namespace NR
{
    class NR_API WindowResizeEvent : public Event
    {
    public:
        WindowResizeEvent(uint32_t width, uint32_t height)
            : mWidth(width), mHeight(height) {}

        inline uint32_t GetWidth() const { return mWidth; }
        inline uint32_t GetHeight() const { return mHeight; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "WindowResizeEvent: " << mWidth << ", " << mHeight;
            return ss.str();
        }

        EVENT_CLASS_TYPE(WindowResize)
        EVENT_CLASS_CATEGORY(EVENTCATEGORYAPPLICATION)

    private:
        uint32_t mWidth, mHeight;
    };

    class NR_API WindowCloseEvent : public Event
    {
    public:
        WindowCloseEvent() {}

        EVENT_CLASS_TYPE(WindowClose)
        EVENT_CLASS_CATEGORY(EVENTCATEGORYAPPLICATION)
    };

    class NR_API AppTickEvent : public Event
    {
        AppTickEvent() {}

        EVENT_CLASS_TYPE(AppTick)
        EVENT_CLASS_CATEGORY(EVENTCATEGORYAPPLICATION)
    };

    class NR_API AppUpdateEvent : public Event
    {
        AppUpdateEvent() {}

        EVENT_CLASS_TYPE(AppUpdate)
        EVENT_CLASS_CATEGORY(EVENTCATEGORYAPPLICATION)
    };

    class NR_API AppRenderEvent : public Event
    {
        AppRenderEvent() {}

        EVENT_CLASS_TYPE(AppRender)
        EVENT_CLASS_CATEGORY(EVENTCATEGORYAPPLICATION)
    };

}