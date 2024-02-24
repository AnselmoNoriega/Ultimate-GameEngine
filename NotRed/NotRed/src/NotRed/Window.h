#pragma once

#include "nrpch.h"

#include "NotRed/Core.h"
#include "NotRed/Events/Event.h"

namespace NR
{
    struct WindowProps
    {
        std::string Title;
        uint32_t Width;
        uint32_t Height;

        WindowProps(
            const std::string& title = "NotRed Engine",
            uint32_t width = 1280,
            uint32_t height = 720
        ) : Title(title), Width(width), Height(height) {}
    };

    class  Window
    {
    public:
        using EventCallbackFn = std::function<void(Event&)>;

        virtual ~Window() {}

        virtual void Update() = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
        virtual void SetVSync(bool enabled) = 0;
        virtual bool VSyncEnabled() const = 0;

        inline virtual void* GetNativeWindow() const = 0;

        static Window* Create(const WindowProps& props = WindowProps());
    };
}