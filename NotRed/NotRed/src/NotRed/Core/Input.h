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

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode keycode);
		static bool IsMouseButtonPressed(int button);

		static float GetMousePositionX();
		static float GetMousePositionY();
		static std::pair<float, float> GetMousePosition();

		static void SetCursorMode(CursorMode mode);
		static CursorMode GetCursorMode();
	};
}