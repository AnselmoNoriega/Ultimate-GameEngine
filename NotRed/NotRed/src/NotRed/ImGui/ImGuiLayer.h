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
        void OnEvent(Event& myEvent) override;

        void Begin();
        void End();

        void SetEventsActive(bool block) { mEventsBlocked = block; }

        void SetThemeColors();

        uint32_t GetActiveWidgetID() const;

    private:
        bool mEventsBlocked = false;
        float mTime = 0.0f;
    };
}

