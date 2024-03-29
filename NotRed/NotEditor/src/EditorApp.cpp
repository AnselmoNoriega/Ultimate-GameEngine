#include <NotRed.h>
#include <NotRed/Core/EntryPoint.h>

#include "EditorLayer.h"

namespace NR
{
    class NotEditor : public Application
    {
    public:
        NotEditor(AppCommandLineArgs args)
            : Application("Not-Red Editor", args)
        {
            PushOverlay(new EditorLayer());
        }

        ~NotEditor() override
        {

        }
    };

    Application* CreateApplication(AppCommandLineArgs args)
    {
        return new NotEditor(args);
    }
}