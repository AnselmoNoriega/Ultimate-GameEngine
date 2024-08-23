#pragma once

#include "KeyCodes.h"

namespace NR
{
	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode keycode);
		static bool IsMouseButtonPressed(int button);

		static float GetMousePositionX();
		static float GetMousePositionY();
		static std::pair<float, float> GetMousePosition();
	};
}