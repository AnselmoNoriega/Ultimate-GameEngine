#pragma once

#include "NotRed/Layer.h"

namespace NR
{
    class NR_API ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer();

        void Attach() override;
        void Detach() override;
        void ImGuiRender() override;

        void Begin();
        void End();

    private:
        float mTime = 0.0f;
    };
}

