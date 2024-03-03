#pragma once

#include "NotRed/Core/Layer.h"

namespace NR
{
    class  ImGuiLayer : public Layer
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

