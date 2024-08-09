#include "nrpch.h"
#include "RendererAPI.h"

#include "Platform/OpenGL/GLRendererAPI.h"

namespace NR
{
    RendererAPI::API RendererAPI::sAPI = RendererAPI::API::None;

    Scope<RendererAPI> RendererAPI::Create(RendererAPI::API api)
    {
        sAPI = api;

        switch (sAPI)
        {
        case RendererAPI::API::None:    NR_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
        case RendererAPI::API::OpenGL:  return CreateScope<GLRendererAPI>();
        default:
            NR_CORE_ASSERT(false, "Unknown RendererAPI!");
            return nullptr;
        }
    }
}