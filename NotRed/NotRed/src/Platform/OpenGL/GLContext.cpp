#include "nrpch.h"
#include "GLContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <gl/GL.h>

namespace NR
{
    GLContext::GLContext(GLFWwindow* windowHandle)
        : mWindowHandle(windowHandle)
    {
        NR_CORE_ASSERT(windowHandle, "Window does not exist.")
    }

    void GLContext::Init()
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

    void GLContext::SwapBuffers()
    {
        NR_PROFILE_FUNCTION();

        glfwSwapBuffers(mWindowHandle);
    }

    GLContext::~GLContext()
    {
    }
}