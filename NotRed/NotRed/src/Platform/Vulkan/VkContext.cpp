#include "nrpch.h"
#include "VkContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace NR
{
    VkContext::VkContext(GLFWwindow* windowHandle)
        : mWindowHandle(windowHandle)
    {
        NR_CORE_ASSERT(windowHandle, "Window does not exist.")
    }

    void VkContext::Init()
    {
        NR_PROFILE_FUNCTION();

        glfwMakeContextCurrent(mWindowHandle);
        int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        NR_CORE_ASSERT(status, "Glad failed to initialize");

        NR_CORE_INFO("OpenGL Info:\n\t\t\t\t\t\tVendor: {0}\n\t\t\t\t\t\tRenderer: {1}\n\t\t\t\t\t\tVersion: {2}",
            (const char*)glGetString(GL_VENDOR),
            (const char*)glGetString(GL_RENDERER),
            (const char*)glGetString(GL_VERSION));
    }

    void VkContext::SwapBuffers()
    {
        NR_PROFILE_FUNCTION();

        glfwSwapBuffers(mWindowHandle);
    }

    VkContext::~VkContext()
    {
    }
}