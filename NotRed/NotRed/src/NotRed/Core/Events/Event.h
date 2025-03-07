#pragma once

#include "nrpch.h"

namespace NR
{
    enum class EventType
    {
        None,
        WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved, WindowTitleBarHitTest,
        AppTick, AppUpdate, AppRender,
        KeyPressed, KeyReleased, KeyTyped,
        MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
    };

    enum EventCategory
    {
        None = (0 << 0),
        EventCategoryApplication = (1 << 0),
        EventCategoryInput = (1 << 1),
        EventCategoryKeyboard = (1 << 2),
        EventCategoryMouse = (1 << 3),
        EventCategoryMouseButton = (1 << 4)
    };

#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::##type; }\
								virtual EventType GetEventType() const override { return GetStaticType(); }\
								virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }

    class Event
    {
    public:
        virtual EventType GetEventType() const = 0;
        virtual const char* GetName() const = 0;
        virtual int GetCategoryFlags() const = 0;
        virtual std::string ToString() const { return GetName(); }

        inline bool IsInCategory(EventCategory category)
        {
            return GetCategoryFlags() & category;
        }

    public:
        bool Handled = false;
    };

    class EventDispatcher
    {
        template<typename T>
        using EventFn = std::function<bool(T&)>;

    public:
        EventDispatcher(Event& event)
            : mEvent(event)
        {
        }

        template<typename T>
        bool Dispatch(EventFn<T> func)
        {
            if (mEvent.GetEventType() == T::GetStaticType())
            {
                mEvent.Handled = func(*(T*)&mEvent);
                return true;
            }
            return false;
        }

    private:
        Event& mEvent;
    };

    inline std::ostream& operator<<(std::ostream& os, const Event& e)
    {
        return os << e.ToString();
    }
}