#include "nrpch.h"
#include "UniformBuffer.h"

#include "NotRed/Renderer/Renderer.h"

#include "NotRed/Platform/OpenGL/GLUniformBuffer.h"
#include "NotRed/Platform/Vulkan/VKUniformBuffer.h"

#include "NotRed/Renderer/RendererAPI.h"

namespace NR
{
	Ref<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t binding)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:     return nullptr;
		case RendererAPIType::OpenGL:  return Ref<GLUniformBuffer>::Create(size, binding);
		case RendererAPIType::Vulkan:  return Ref<VKUniformBuffer>::Create(size, binding);
		default:
			NR_CORE_ASSERT(false, "Unknown RendererAPI!");
			return nullptr;
		}
	}
}