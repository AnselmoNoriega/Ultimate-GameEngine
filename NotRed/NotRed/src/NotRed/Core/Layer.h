#pragma once

#include "NotRed/Core/Core.h"
#include "NotRed/Events/Event.h"

namespace NR
{
    class Layer
    {
    public:
        Layer(const std::string& name = "Layer");
        virtual ~Layer();

        virtual void Attach() {}
        virtual void Detach() {}
        virtual void Update(float deltaTime) {}
        virtual void ImGuiRender() {}
        virtual void OnEvent(Event& myEvent) {}

        inline const std::string& GetName() const { return mDebugName; }

    protected:
        std::string mDebugName;
    };
}

