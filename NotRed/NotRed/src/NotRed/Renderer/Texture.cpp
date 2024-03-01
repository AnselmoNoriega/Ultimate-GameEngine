#include "nrpch.h"
#include "Texture.h"

#include "Renderer.h"
#include "Platform/OpenGL/GLTexture.h"

namespace NR
{
    Ref<Texture2D> Texture2D::Create(const std::string& path)
    {
        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::None: NR_CORE_ASSERT(false, "Renderer API \"None\" is currently not supported!");
        case RendererAPI::API::OpenGL: return std::make_shared<GLTexture2D>(path);
        default:
        {
            NR_CORE_ASSERT(false, "Unknown RenderAPI!");
            return nullptr;
        }
        }
    }
}