#include <NotRed.h>
#include <NotRed/Core/EntryPoint.h>

#include "EditorLayer.h"

namespace NR
{
    class NotEditor : public Application
    {
    public:
        NotEditor(const ApplicationSpecification& spec)
            : Application(spec)
        {
            PushOverlay(new EditorLayer());
        }
    };

    Application* CreateApplication(AppCommandLineArgs args)
    {
        ApplicationSpecification spec;
        spec.Name = "NotRed";
        spec.CommandLineArgs = args;

        return new NotEditor(spec);
    }
}