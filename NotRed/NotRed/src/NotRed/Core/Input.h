#pragma once

#include "NotRed/Core/Core.h"
#include "NotRed/Core/KeyCodes.h"

namespace NR
{
    class  Input
    {
    public:
        static bool IsKeyPressed(KeyCode keycode);
        static bool IsMouseButtonPressed(KeyCode button);

        static std::pair<float, float> GetMousePosition();
    };
}