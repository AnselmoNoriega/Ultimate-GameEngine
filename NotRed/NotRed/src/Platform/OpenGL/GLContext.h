#pragma once

#include "NotRed/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace NR
{
    class GLContext : public GraphicsContext
    {
    public:
        GLContext(GLFWwindow* windowHandle);
        ~GLContext() override;

        void Init() override;
        void SwapBuffers() override;

    private:
        GLFWwindow* mWindowHandle;
    };
}

