#include "nrpch.h"
#include "Pipeline.h"

#include "Renderer.h"

#include "NotRed/Platform/Vulkan/VKPipeline.h"

#include "NotRed/Renderer/RendererAPI.h"

namespace NR
{
    Ref<Pipeline> Pipeline::Create(const PipelineSpecification& spec)
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None:    return nullptr;
        case RendererAPIType::Vulkan:  return Ref<VKPipeline>::Create(spec);
        default:
        {
            NR_CORE_ASSERT(false, "Unknown RendererAPI");
            return nullptr;
        }
        }
    }
}