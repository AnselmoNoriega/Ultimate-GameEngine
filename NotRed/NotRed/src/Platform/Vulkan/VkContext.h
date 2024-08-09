#pragma once

#include "NotRed/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace NR
{
    class VkContext : public GraphicsContext
    {
    public:
        VkContext(GLFWwindow* windowHandle);
        ~VkContext() override;

        void Init() override;
        void SwapBuffers() override;

    private:
        GLFWwindow* mWindowHandle;
    };
}

