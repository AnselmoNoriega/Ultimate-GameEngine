#include "nrpch.h"
#include "PipelineCompute.h"

#include "NotRed/Renderer/RendererAPI.h"
#include "NotRed/Platform/Vulkan/VKComputePipeline.h"

namespace NR
{
    Ref<PipelineCompute> PipelineCompute::Create(Ref<Shader> computeShader)
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::None: return nullptr;
        case RendererAPIType::Vulkan: return Ref<VKComputePipeline>::Create(computeShader);
        default:
            NR_CORE_ASSERT(false, "Unknown RendererAPI");
            return nullptr;
        }
    }
}