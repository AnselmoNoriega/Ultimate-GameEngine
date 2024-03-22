#pragma once

#include "NotRed/Core/Core.h"
#include "NotRed/Core/KeyCodes.h"
#include "NotRed/Core/MouseButtonCodes.h"

namespace NR
{
    class  Input
    {
    public:
        static bool IsKeyPressed(KeyCode keycode);
        static bool IsMouseButtonPressed(MouseCode button);

        static std::pair<float, float> GetMousePosition();
    };
}