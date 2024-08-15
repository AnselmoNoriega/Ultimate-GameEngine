#pragma once

namespace NR
{
	class Input
	{
	public:
		static bool IsKeyPressed(int keycode) { return sInstance->IsKeyPressedImpl(keycode); }

		inline static bool IsMouseButtonPressed(int button) { return sInstance->IsMouseButtonPressedImpl(button); }
		inline static float GetMouseX() { return sInstance->GetMouseXImpl(); }
		inline static float GetMouseY() { return sInstance->GetMouseYImpl(); }

	protected:
		virtual bool IsKeyPressedImpl(int keycode) = 0;
		virtual bool IsMouseButtonPressedImpl(int button) = 0;
		virtual float GetMouseXImpl() = 0;
		virtual float GetMouseYImpl() = 0;

	private:
		static Input* sInstance;
	};
}