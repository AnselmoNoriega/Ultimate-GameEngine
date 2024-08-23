#pragma once

namespace NR
{
	typedef enum class KeyCode : uint16_t
	{
		// From glfw3.h
		Space                            = 32,
		Apostrophe                       = 39, /* ' */
		Comma                            = 44, /* , */
		Minus                            = 45, /* - */
		Period                           = 46, /* . */
		Slash                            = 47, /* / */

		D0                               = 48, /* 0 */
		D1                               = 49, /* 1 */
		D2                               = 50, /* 2 */
		D3                               = 51, /* 3 */
		D4                               = 52, /* 4 */
		D5                               = 53, /* 5 */
		D6                               = 54, /* 6 */
		D7                               = 55, /* 7 */
		D8                               = 56, /* 8 */
		D9                               = 57, /* 9 */

		Semicolon                        = 59, /* ; */
		Equal                            = 61, /* = */

		A                                = 65,
		B                                = 66,
		C                                = 67,
		D                                = 68,
		E                                = 69,
		F                                = 70,
		G                                = 71,
		H                                = 72,
		I                                = 73,
		J                                = 74,
		K                                = 75,
		L                                = 76,
		M                                = 77,
		N                                = 78,
		O                                = 79,
		P                                = 80,
		Q                                = 81,
		R                                = 82,
		S                                = 83,
		T                                = 84,
		U                                = 85,
		V                                = 86,
		W                                = 87,
		X                                = 88,
		Y                                = 89,
		Z                                = 90,

		LeftBracket                      = 91,  /* [ */
		Backslash                        = 92,  /* \ */
		RightBracket                     = 93,  /* ] */
		GraveAccent                      = 96,  /* ` */

		World1                           = 161, /* non-US #1 */
		World2                           = 162, /* non-US #2 */

		/* Function keys */
		Escape                           = 256,
		Enter                            = 257,
		Tab                              = 258,
		Backspace                        = 259,
		Insert                           = 260,
		Delete                           = 261,
		Right                            = 262,
		Left                             = 263,
		Down                             = 264,
		Up                               = 265,
		PageUp                           = 266,
		PageDown                         = 267,
		Home                             = 268,
		End                              = 269,
		CapsLock                         = 280,
		ScrollLock                       = 281,
		NumLock                          = 282,
		PrintScreen                      = 283,
		Pause                            = 284,
		F1                               = 290,
		F2                               = 291,
		F3                               = 292,
		F4                               = 293,
		F5                               = 294,
		F6                               = 295,
		F7                               = 296,
		F8                               = 297,
		F9                               = 298,
		F10                              = 299,
		F11                              = 300,
		F12                              = 301,
		F13                              = 302,
		F14                              = 303,
		F15                              = 304,
		F16                              = 305,
		F17                              = 306,
		F18                              = 307,
		F19                              = 308,
		F20                              = 309,
		F21                              = 310,
		F22                              = 311,
		F23                              = 312,
		F24                              = 313,
		F25                              = 314,

		/* Keypad */
		KP0                              = 320,
		KP1                              = 321,
		KP2                              = 322,
		KP3                              = 323,
		KP4                              = 324,
		KP5                              = 325,
		KP6                              = 326,
		KP7                              = 327,
		KP8                              = 328,
		KP9                              = 329,
		KPDecimal                        = 330,
		KPDivide                         = 331,
		KPMultiply                       = 332,
		KPSubtract                       = 333,
		KPAdd                            = 334,
		KPEnter                          = 335,
		KPEqual                          = 336,

		LeftShift                        = 340,
		LeftControl                      = 341,
		LeftAlt                          = 342,
		LeftSuper                        = 343,
		RightShift                       = 344,
		RightControl                     = 345,
		RightAlt                         = 346,
		RightSuper                       = 347,
		Menu                             = 348
	} Key;

	inline std::ostream& operator<<(std::ostream& os, KeyCode keyCode)
	{
		os << static_cast<int32_t>(keyCode);
		return os;
	}
}

// From glfw3.h
#define NR_KEY_SPACE           NR::Key::Space
#define NR_KEY_APOSTROPHE      NR::Key::Apostrophe    /* ' */
#define NR_KEY_COMMA           NR::Key::Comma         /* , */
#define NR_KEY_MINUS           NR::Key::Minus         /* - */
#define NR_KEY_PERIOD          NR::Key::Period        /* . */
#define NR_KEY_SLASH           NR::Key::Slash         /* / */
#define NR_KEY_0               NR::Key::D0
#define NR_KEY_1               NR::Key::D1
#define NR_KEY_2               NR::Key::D2
#define NR_KEY_3               NR::Key::D3
#define NR_KEY_4               NR::Key::D4
#define NR_KEY_5               NR::Key::D5
#define NR_KEY_6               NR::Key::D6
#define NR_KEY_7               NR::Key::D7
#define NR_KEY_8               NR::Key::D8
#define NR_KEY_9               NR::Key::D9
#define NR_KEY_SEMICOLON       NR::Key::Semicolon     /* ; */
#define NR_KEY_EQUAL           NR::Key::Equal         /*                                                                  = */
#define NR_KEY_A               NR::Key::A
#define NR_KEY_B               NR::Key::B
#define NR_KEY_C               NR::Key::C
#define NR_KEY_D               NR::Key::D
#define NR_KEY_E               NR::Key::E
#define NR_KEY_F               NR::Key::F
#define NR_KEY_G               NR::Key::G
#define NR_KEY_H               NR::Key::H
#define NR_KEY_I               NR::Key::I
#define NR_KEY_J               NR::Key::J
#define NR_KEY_K               NR::Key::K
#define NR_KEY_L               NR::Key::L
#define NR_KEY_M               NR::Key::M
#define NR_KEY_N               NR::Key::N
#define NR_KEY_O               NR::Key::O
#define NR_KEY_P               NR::Key::P
#define NR_KEY_Q               NR::Key::Q
#define NR_KEY_R               NR::Key::R
#define NR_KEY_S               NR::Key::S
#define NR_KEY_T               NR::Key::T
#define NR_KEY_U               NR::Key::U
#define NR_KEY_V               NR::Key::V
#define NR_KEY_W               NR::Key::W
#define NR_KEY_X               NR::Key::X
#define NR_KEY_Y               NR::Key::Y
#define NR_KEY_Z               NR::Key::Z
#define NR_KEY_LEFT_BRACKET    NR::Key::LeftBracket   /* [ */
#define NR_KEY_BACKSLASH       NR::Key::Backslash     /* \ */
#define NR_KEY_RIGHT_BRACKET   NR::Key::RightBracket  /* ] */
#define NR_KEY_GRAVE_ACCENT    NR::Key::GraveAccent   /* ` */
#define NR_KEY_WORLD_1         NR::Key::World1        /* non-US #1 */
#define NR_KEY_WORLD_2         NR::Key::World2        /* non-US #2 */

/* Function keys */
#define NR_KEY_ESCAPE          NR::Key::Escape
#define NR_KEY_ENTER           NR::Key::Enter
#define NR_KEY_TAB             NR::Key::Tab
#define NR_KEY_BACKSPACE       NR::Key::Backspace
#define NR_KEY_INSERT          NR::Key::Insert
#define NR_KEY_DELETE          NR::Key::Delete
#define NR_KEY_RIGHT           NR::Key::Right
#define NR_KEY_LEFT            NR::Key::Left
#define NR_KEY_DOWN            NR::Key::Down
#define NR_KEY_UP              NR::Key::Up
#define NR_KEY_PAGE_UP         NR::Key::PageUp
#define NR_KEY_PAGE_DOWN       NR::Key::PageDown
#define NR_KEY_HOME            NR::Key::Home
#define NR_KEY_END             NR::Key::End
#define NR_KEY_CAPS_LOCK       NR::Key::CapsLock
#define NR_KEY_SCROLL_LOCK     NR::Key::ScrollLock
#define NR_KEY_NUM_LOCK        NR::Key::NumLock
#define NR_KEY_PRINT_SCREEN    NR::Key::PrintScreen
#define NR_KEY_PAUSE           NR::Key::Pause
#define NR_KEY_F1              NR::Key::F1
#define NR_KEY_F2              NR::Key::F2
#define NR_KEY_F3              NR::Key::F3
#define NR_KEY_F4              NR::Key::F4
#define NR_KEY_F5              NR::Key::F5
#define NR_KEY_F6              NR::Key::F6
#define NR_KEY_F7              NR::Key::F7
#define NR_KEY_F8              NR::Key::F8
#define NR_KEY_F9              NR::Key::F9
#define NR_KEY_F10             NR::Key::F10
#define NR_KEY_F11             NR::Key::F11
#define NR_KEY_F12             NR::Key::F12
#define NR_KEY_F13             NR::Key::F13
#define NR_KEY_F14             NR::Key::F14
#define NR_KEY_F15             NR::Key::F15
#define NR_KEY_F16             NR::Key::F16
#define NR_KEY_F17             NR::Key::F17
#define NR_KEY_F18             NR::Key::F18
#define NR_KEY_F19             NR::Key::F19
#define NR_KEY_F20             NR::Key::F20
#define NR_KEY_F21             NR::Key::F21
#define NR_KEY_F22             NR::Key::F22
#define NR_KEY_F23             NR::Key::F23
#define NR_KEY_F24             NR::Key::F24
#define NR_KEY_F25             NR::Key::F25

/* Keypad  */
#define NR_KEY_KP_0            NR::Key::KP0
#define NR_KEY_KP_1            NR::Key::KP1
#define NR_KEY_KP_2            NR::Key::KP2
#define NR_KEY_KP_3            NR::Key::KP3
#define NR_KEY_KP_4            NR::Key::KP4
#define NR_KEY_KP_5            NR::Key::KP5
#define NR_KEY_KP_6            NR::Key::KP6
#define NR_KEY_KP_7            NR::Key::KP7
#define NR_KEY_KP_8            NR::Key::KP8
#define NR_KEY_KP_9            NR::Key::KP9
#define NR_KEY_KP_DECIMAL      NR::Key::KPDecimal
#define NR_KEY_KP_DIVIDE       NR::Key::KPDivide
#define NR_KEY_KP_MULTIPLY     NR::Key::KPMultiply
#define NR_KEY_KP_SUBTRACT     NR::Key::KPSubtract
#define NR_KEY_KP_ADD          NR::Key::KPAdd
#define NR_KEY_KP_ENTER        NR::Key::KPEnter
#define NR_KEY_KP_EQUAL        NR::Key::KPEqual

#define NR_KEY_LEFT_SHIFT      NR::Key::LeftShift
#define NR_KEY_LEFT_CONTROL    NR::Key::LeftControl
#define NR_KEY_LEFT_ALT        NR::Key::LeftAlt
#define NR_KEY_LEFT_SUPER      NR::Key::LeftSuper
#define NR_KEY_RIGHT_SHIFT     NR::Key::RightShift
#define NR_KEY_RIGHT_CONTROL   NR::Key::RightControl
#define NR_KEY_RIGHT_ALT       NR::Key::RightAlt
#define NR_KEY_RIGHT_SUPER     NR::Key::RightSuper
#define NR_KEY_MENU            NR::Key::Menu

#define NR_MOUSE_BUTTON_LEFT    0
#define NR_MOUSE_BUTTON_RIGHT   1
#define NR_MOUSE_BUTTON_MIDDLE  2