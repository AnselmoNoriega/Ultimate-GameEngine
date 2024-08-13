#pragma once

#include "NotRed/Core/Window.h"

namespace NR
{
	class WinWindow : public Window
	{
	public:
		WinWindow(const WindowProps& props);
		virtual ~WinWindow();

		inline void SetEventCallback(const EventCallbackFn& callback) override { mEventCallbackFn = callback; }

		inline unsigned int GetWidth() const override { return mWidth; }
		inline unsigned int GetHeight() const override { return mHeight; }

	protected:
		virtual void Init(const WindowProps& props) override;
		virtual void Shutdown() override;

	private:
		std::string mTitle;
		unsigned int mWidth, mHeight;

		EventCallbackFn mEventCallbackFn;
	};
}

