#include "nrpch.h"
#include "OpenGLContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <gl/GL.h>

namespace NR
{
    OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
        : mWindowHandle(windowHandle)
    {
        NR_CORE_ASSERT(windowHandle, "Window does not exist.")
    }

    void OpenGLContext::Init()
    {
        glfwMakeContextCurrent(mWindowHandle);
        int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        NR_CORE_ASSERT(status, "Glad failed to initialize");

        NR_CORE_INFO("OpenGL Info:\n\t\t\t\t\t\tVendor: {0}\n\t\t\t\t\t\tRenderer: {1}\n\t\t\t\t\t\tVersion: {2}",
            (const char*)glGetString(GL_VENDOR),
            (const char*)glGetString(GL_RENDERER),
            (const char*)glGetString(GL_VERSION));
    }

    void OpenGLContext::SwapBuffers()
    {
        glfwSwapBuffers(mWindowHandle);
    }

    OpenGLContext::~OpenGLContext()
    {
    }
}