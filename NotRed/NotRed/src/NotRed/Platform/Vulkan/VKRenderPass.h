#pragma once

#include "NotRed/Renderer/RenderPass.h"

namespace NR
{
	class VKRenderPass : public RenderPass
	{
	public:
		VKRenderPass(const RenderPassSpecification& spec);
		~VKRenderPass() override;

		RenderPassSpecification& GetSpecification() override { return mSpecification; }
		const RenderPassSpecification& GetSpecification() const override { return mSpecification; }
		
	private:
		RenderPassSpecification mSpecification;
	};
}