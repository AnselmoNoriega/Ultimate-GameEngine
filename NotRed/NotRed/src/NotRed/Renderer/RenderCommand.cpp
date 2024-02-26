#include "nrpch.h"
#include "RenderCommand.h"

#include "Platform/OpenGL/GLRendererAPI.h"

namespace NR
{
    RendererAPI* RenderCommand::sRendererAPI = new GLRendererAPI;
}