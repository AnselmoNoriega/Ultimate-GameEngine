#pragma once

#include "NotRed/Input.h"

namespace NR
{
    class WindowsInput : public Input
    {
    protected:
        bool IsKeyPressedImpl(int keycode) override;
        bool IsMouseBtnPressedImpl(int button) override;
        std::pair<float, float> GetMousePosImpl() override;
    };
}

