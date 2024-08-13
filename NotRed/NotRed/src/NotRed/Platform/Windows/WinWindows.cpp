#include "WinWindows.h"

namespace NR
{
	Window* Window::Create(const WindowProps& props)
	{
		return new WinWindow(props);
	}

	WinWindow::WinWindow(const WindowProps& props)
		: Window(props)
	{
	}

	WinWindow::~WinWindow()
	{
	}

	void WinWindow::Init(const WindowProps& props)
	{
		mTitle = props.Title;
		mWidth = props.Width;
		mHeight = props.Height;
	}

	void WinWindow::Shutdown()
	{
	}
}