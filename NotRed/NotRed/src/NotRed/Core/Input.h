#pragma once

#include "KeyCodes.h"

namespace NR
{
	enum class CursorMode
	{
		Normal,
		Hidden,
		Locked
	};

	typedef enum class MouseButton : uint16_t
	{
		Button0,
		Button1,
		Button2,
		Button3,
		Button4,
		Button5,
		Left = Button0,
		Right = Button1,
		Middle = Button2
	};

	inline std::ostream& operator<<(std::ostream& os, MouseButton button)
	{
		os << static_cast<int32_t>(button);
		return os;
	}

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode keycode);
		static bool IsMouseButtonPressed(MouseButton button);

		static float GetMousePositionX();
		static float GetMousePositionY();
		static std::pair<float, float> GetMousePosition();

		static void SetCursorMode(CursorMode mode);
		static CursorMode GetCursorMode();
	};
}