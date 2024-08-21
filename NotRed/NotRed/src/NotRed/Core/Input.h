#pragma once

namespace NR
{
	class Input
	{
	public:
		static bool IsKeyPressed(int keycode);
		static bool IsMouseButtonPressed(int button);

		static float GetMousePositionX();
		static float GetMousePositionY();
		static std::pair<float, float> GetMousePosition();
	};
}