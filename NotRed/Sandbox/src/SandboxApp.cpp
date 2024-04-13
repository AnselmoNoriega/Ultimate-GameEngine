#include <NotRed.h>
#include <NotRed/Core/EntryPoint.h>

#include "Sandbox2D.h"

class Sandbox : public NR::Application
{
public:
    Sandbox(const NR::ApplicationSpecification& specification)
        : NR::Application(specification)
    {
        PushOverlay(new Sandbox2D());
    }

    ~Sandbox() override
    {

    }
};

NR::Application* NR::CreateApplication(NR::AppCommandLineArgs args)
{
    NR::ApplicationSpecification spec;
    spec.Name = "Sandbox";
    spec.WorkingDirectory = "../NotEditor";
    spec.CommandLineArgs = args;

    return new Sandbox(spec);
}