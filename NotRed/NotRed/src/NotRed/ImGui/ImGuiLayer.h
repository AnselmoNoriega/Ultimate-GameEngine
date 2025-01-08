#pragma once

#include "nrpch.h"
#include "NotRed/Core/Layer.h"

namespace NR
{
    class ImGuiLayer : public Layer
    {
    public:
        virtual void Begin() = 0;
        virtual void End() = 0;

        void SetDarkThemeColors();
        void SetDarkThemeV2Colors();

        static ImGuiLayer* Create();
    };
}