#pragma once

#include "NotRed/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace NR
{
    class OpenGLContext : public GraphicsContext
    {
    public:
        OpenGLContext(GLFWwindow* windowHandle);
        ~OpenGLContext() override;

        void Init() override;
        void SwapBuffers() override;

    private:
        GLFWwindow* mWindowHandle;
    };
}

