#pragma once

#include "NotRed/Core.h"
#include "NotRed/Events/Event.h"

namespace NR
{
    class NR_API Layer
    {
    public:
        Layer(const std::string& name = "Layer");
        virtual ~Layer();

        virtual void Attach() {}
        virtual void Detach() {}
        virtual void Update() {}
        virtual void ImGuiRender() {}
        virtual void OnEvent(Event& event) {}

        inline const std::string& GetName() const { return mDebugName; }

    protected:
        std::string mDebugName;
    };
}

