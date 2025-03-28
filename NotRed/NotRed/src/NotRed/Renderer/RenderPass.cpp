#include "nrpch.h"
#include "RenderPass.h"

#include "Renderer.h"

#include "NotRed/Platform/Vulkan/VKRenderPass.h"

#include "NotRed/Renderer/RendererAPI.h"

namespace NR
{
	Ref<RenderPass> RenderPass::Create(const RenderPassSpecification& spec)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:    NR_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPIType::Vulkan:  return Ref<VKRenderPass>::Create(spec);
		default:
		{
			NR_CORE_ASSERT(false, "Unknown RendererAPI!");
			return nullptr;
		}
		}
	}
}