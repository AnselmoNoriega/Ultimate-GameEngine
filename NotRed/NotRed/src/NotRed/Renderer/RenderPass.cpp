#include "nrpch.h"
#include "RenderPass.h"

#include "Renderer.h"

#include "Platform/OpenGL/GLRenderPass.h"
//#include "NotRed/Platform/Vulkan/VulkanRenderPass.h"

#include "NotRed/Renderer/RendererAPI.h"

namespace NR
{
	Ref<RenderPass> RenderPass::Create(const RenderPassSpecification& spec)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None:    NR_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		//case RendererAPI::API::Vulkan:  return Ref<VulkanRenderPass>::Create(spec);
		case RendererAPI::API::OpenGL:  return CreateRef<GLRenderPass>(spec);
		default:
		{
			NR_CORE_ASSERT(false, "Unknown RendererAPI!");
			return nullptr;
		}
		}
	}

}