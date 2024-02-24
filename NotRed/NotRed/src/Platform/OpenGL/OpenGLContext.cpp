#include "nrpch.h"
#include "OpenGLContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

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
    }

    void OpenGLContext::SwapBuffers()
    {
        glfwSwapBuffers(mWindowHandle);
    }

    OpenGLContext::~OpenGLContext()
    {
    }
}