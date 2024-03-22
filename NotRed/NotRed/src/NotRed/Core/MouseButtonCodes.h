#pragma once

// From glfw3.h
namespace NR
{
    typedef enum class MouseCode : uint16_t
    {
        Button_1         = 0,
        Button_2         = 1,
        Button_3         = 2,
        Button_4         = 3,
        Button_5         = 4,
        Button_6         = 5,
        Button_7         = 6,
        Button_8         = 7,
        ButtonLast      = Button_8,
        ButtonLeft      = Button_1,
        ButtonRight     = Button_2,
        ButtonMiddle    = Button_3
    };
}