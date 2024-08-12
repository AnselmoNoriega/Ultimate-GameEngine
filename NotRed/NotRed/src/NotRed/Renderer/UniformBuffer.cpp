#include "nrpch.h"
#include "UniformBuffer.h"

#include "NotRed/Renderer/Renderer.h"

#include "Platform/OpenGL/GLUniformBuffer.h"
//#include "NotRed/Platform/Vulkan/VulkanUniformBuffer.h"

#include "NotRed/Renderer/RendererAPI.h"

namespace NR
{
	Ref<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t binding)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    NR_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<GLUniformBuffer>(size, binding);
		default:
			NR_CORE_ASSERT(false, "Unknown RendererAPI!"); return nullptr;
		}

	}

}