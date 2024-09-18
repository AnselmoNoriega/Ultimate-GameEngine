#include "nrpch.h"
#include "VKRenderPass.h"

namespace NR
{
	VKRenderPass::VKRenderPass(const RenderPassSpecification& spec)
		: mSpecification(spec)
	{
	}

	VKRenderPass::~VKRenderPass()
	{
	}
}