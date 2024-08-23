#pragma once

#include "NotRed/Renderer/RenderPass.h"

namespace NR
{
	class GLRenderPass : public RenderPass
	{
	public:
		GLRenderPass(const RenderPassSpecification& spec);
		~GLRenderPass() override;

		RenderPassSpecification& GetSpecification() override { return mSpecification; }
		const RenderPassSpecification& GetSpecification() const override { return mSpecification; }

	private:
		RenderPassSpecification mSpecification;
	};

}