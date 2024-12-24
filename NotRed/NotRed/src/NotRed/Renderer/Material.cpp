#include "nrpch.h"
#include "Material.h"

#include "NotRed/Platform/OpenGL/GLMaterial.h"
#include "NotRed/Platform/Vulkan/VKMaterial.h"

#include "NotRed/Renderer/RendererAPI.h"

namespace NR
{
	Ref<Material> Material::Create(const Ref<Shader>& shader, const std::string& name)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return nullptr;
		case RendererAPIType::OpenGL: return Ref<GLMaterial>::Create(shader, name);
		case RendererAPIType::Vulkan: return Ref<VKMaterial>::Create(shader, name);
		default:
		{
			NR_CORE_ASSERT(false, "Unknown RendererAPI");
			return nullptr;
		}
		}
	}

	Ref<Material> Material::Copy(const Ref<Material>& other, const std::string& name)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return nullptr;
		case RendererAPIType::Vulkan: return Ref<VKMaterial>::Create(other, name);
		default:
		{
			NR_CORE_ASSERT(false, "Unknown RendererAPI");
			return nullptr;
		}
		}
	}
}