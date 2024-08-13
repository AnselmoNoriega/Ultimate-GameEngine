#include "nrpch.h"
#include "Pipeline.h"

#include "Renderer.h"

//#include "Platform/OpenGL/GLPipeline.h"

namespace NR
{
    Ref<Pipeline> Pipeline::Create(const PipelineSpecification& spec)
    {
        //switch (Renderer::GetAPI())
        //{
        //case RendererAPI::API::None:    return nullptr;
        //case RendererAPI::API::OpenGL:  return CreateRef<GLPipeline>(spec);
        //default:
        //{
        //    NR_CORE_ASSERT(false, "Unknown RendererAPI");
        //    return nullptr;
        //}
        //}
    }

}