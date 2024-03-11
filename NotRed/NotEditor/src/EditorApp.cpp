#include <NotRed.h>
#include <NotRed/Core/EntryPoint.h>

#include "EditorLayer.h"

namespace NR
{
    class NotEditor : public Application
    {
    public:
        NotEditor()
            : Application("Not-Red Editor")
        {
            PushOverlay(new EditorLayer());
        }

        ~NotEditor() override
        {

        }
    };

    Application* CreateApplication()
    {
        return new NotEditor();
    }
}