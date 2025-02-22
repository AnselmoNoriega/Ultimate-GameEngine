#pragma once

#include <map>

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

	struct Controller
	{
		int ID;
		std::string Name;
		std::map<int, bool> ButtonStates;
		std::map<int, float> AxisStates;
		std::map<int, uint8_t> HatStates;
	};

	class Input
	{
	public:
		static void Update();

		static bool IsKeyPressed(KeyCode keycode);
		static bool IsMouseButtonPressed(MouseButton button);

		static float GetMousePositionX();
		static float GetMousePositionY();
		static std::pair<float, float> GetMousePosition();

		static void SetCursorMode(CursorMode mode);
		static CursorMode GetCursorMode();

		// Controllers
		static bool IsControllerPresent(int id);
		static std::vector<int> GetConnectedControllerIDs();
		static const Controller* GetController(int id);
		static std::string_view GetControllerName(int id);
		static bool IsControllerButtonPressed(int controllerID, int button);
		static float GetControllerAxis(int controllerID, int axis);
		static uint8_t GetControllerHat(int controllerID, int hat);

		static const std::map<int, Controller>& GetControllers() { return sControllers; }
	
	private:
		inline static std::map<int, Controller> sControllers;
	};
}