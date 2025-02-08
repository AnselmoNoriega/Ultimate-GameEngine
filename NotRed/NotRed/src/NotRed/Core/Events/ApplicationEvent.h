#pragma once

#include "Event.h"

#include <sstream>

namespace NR
{
    class WindowCloseEvent : public Event
    {
    public:
        WindowCloseEvent() = default;

        EVENT_CLASS_TYPE(WindowClose)
            EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class WindowResizeEvent : public Event
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

    class WindowTitleBarHitTestEvent : public Event
    {
    public:
        WindowTitleBarHitTestEvent(int x, int y, int& hit)
            : m_X(x), m_Y(y), mHit(hit) {}

        inline int GetX() const { return m_X; }
        inline int GetY() const { return m_Y; }
        inline void SetHit(bool hit) { mHit = (int)hit; }

        EVENT_CLASS_TYPE(WindowTitleBarHitTest)
            EVENT_CLASS_CATEGORY(EventCategoryApplication)
    private:
        int m_X;
        int m_Y;
        int& mHit;
    };

    class AppTickEvent : public Event
    {
    public:
        AppTickEvent() = default;

        EVENT_CLASS_TYPE(AppTick)
            EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class AppUpdateEvent : public Event
    {
    public:
        AppUpdateEvent() = default;

        EVENT_CLASS_TYPE(AppUpdate)
            EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class AppRenderEvent : public Event
    {
    public:
        AppRenderEvent() = default;

        EVENT_CLASS_TYPE(AppRender)
            EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };
}