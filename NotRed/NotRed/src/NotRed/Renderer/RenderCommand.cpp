#include "nrpch.h"
#include "RenderCommand.h"

#include "Platform/OpenGL/GLRendererAPI.h"

namespace NR
{
    Ref<RendererAPI> RenderCommand::sRendererAPI = CreateRef<GLRendererAPI>();
}