#include <NotRed.h>
#include <NotRed/Core/EntryPoint.h>

#include "Sandbox2D.h"

class Sandbox : public NR::Application
{
public:
    Sandbox()
    {
        PushOverlay(new Sandbox2D());
    }

    ~Sandbox() override
    {

    }
};

NR::Application* NR::CreateApplication()
{
    return new Sandbox();
}