#include "nrpch.h"
#include "Framebuffer.h"

#include "NotRed/Renderer/Renderer.h"

#include "Platform/OpenGL/GLFramebuffer.h"

namespace NR
{
    Ref<Framebuffer> Framebuffer::Create(const FramebufferStruct& spec)
    {
        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::None: NR_CORE_ASSERT(false, "RendererAPI \"None\" is currently not supported!");
        case RendererAPI::API::OpenGL: return CreateRef<GLFramebuffer>(spec);
        default:
        {
            NR_CORE_ASSERT(false, "Unknown Renderer API!");
            return nullptr;
        }
        }

        return nullptr;
    }
}
