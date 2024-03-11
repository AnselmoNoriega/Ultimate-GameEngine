#pragma once

#include "NotRed/Core/Core.h"

namespace NR
{
    class  Input
    {
    public:
        static bool IsKeyPressed(int keycode);
        static bool IsMouseButtonPressed(int button);

        static std::pair<float, float> GetMousePosition();
    };
}