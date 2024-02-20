#pragma once

#include "nrpch.h"
#include "NotRed/Core.h"

namespace NR
{
    enum class EventType
    {
        None,
        WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
        AppTick, AppUpdate, AppRender,
        KeyPressed, KeyReleased,
        MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
    };

    enum EventCategory
    {
        NONE,
        EVENTCATEGORYAPPLICATION = (1 << 0),
        EVENTCATEGORYINPUT = (1 << 1),
        EVENTCATEGORYKEYBOARD = (1 << 2),
        EVENTCATEGORYMOUSE = (1 << 3),
        EVENTCATEGORYMOUSEBUTTON = (1 << 4)
    };

#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() {return EventType::##type;}\
                               virtual EventType GetEventType() const override { return GetStaticType(); }

#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }

    class NR_API Event
    {
    public:
        bool Handled = false;

    public:
        virtual EventType GetEventType() const = 0;
        virtual int GetCategoryFlags() const = 0;
        virtual std::string ToString() const = 0;

        inline bool IsInCategory(EventCategory category)
        {
            return GetCategoryFlags() & category;
        }

    private:
        friend class EventDispatcher;
    };

    class EventDispatcher
    {
    public:
        EventDispatcher(Event& event)
            : mEvent(event) {}

        template <typename T>
        bool Dispatch(std::function<bool(T&)> func)
        {
            if (mEvent.GetEventType() == T::GetStaticType())
            {
                mEvent.Handled = func(*static_cast<T*>(&mEvent));
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