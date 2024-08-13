#pragma once

#include <functional>

#include "NotRed/Core/Core.h"
#include "NotRed/Core/Events/Event.h"

namespace NR
{
	struct WindowProps
	{
		std::string Title;
		unsigned int Width;
		unsigned int Height;

		WindowProps(const std::string& title = "NotRed Engine",
			unsigned int width = 1280,
			unsigned int height = 720)
			: Title(title), Width(width), Height(height)
		{
		}
	};

	class NOT_RED_API Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		Window(const WindowProps& props);
		virtual ~Window() {}

		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual unsigned int GetWidth() const = 0;
		virtual unsigned int GetHeight() const = 0;

		static Window* Create(const WindowProps& props = WindowProps());

	protected:
		virtual void Init(const WindowProps& props) = 0;
		virtual void Shutdown() = 0;
	};

}