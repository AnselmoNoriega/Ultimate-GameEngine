#pragma once

#include "NotRed/Core/Core.h"

namespace NR
{
    class  Input
    {
    public:
        inline static bool IsKeyPressed(int keycode) { return sInstance->IsKeyPressedImpl(keycode); }
        inline static bool IsMouseButtonPressed(int button) { return sInstance->IsMouseBtnPressedImpl(button); }
        inline static std::pair<float, float> GetMousePosition() { return sInstance->GetMousePosImpl(); }

    protected:
        virtual bool IsKeyPressedImpl(int keycode) = 0;
        virtual bool IsMouseBtnPressedImpl(int button) = 0;
        virtual std::pair<float, float> GetMousePosImpl() = 0;

    private:
        static Input* sInstance;
    };
}