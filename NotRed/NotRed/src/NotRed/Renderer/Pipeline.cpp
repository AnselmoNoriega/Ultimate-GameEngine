#include "nrpch.h"
#include "Pipeline.h"

#include "Renderer.h"

#include "NotRed/Platform/OpenGL/GLPipeline.h"

namespace NR
{
    Ref<Pipeline> Pipeline::Create(const PipelineSpecification& spec)
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None:    return nullptr;
        case RendererAPIType::OpenGL:  return Ref<GLPipeline>::Create(spec);
        default:
        {
            NR_CORE_ASSERT(false, "Unknown RendererAPI");
            return nullptr;
        }
        }
    }
}