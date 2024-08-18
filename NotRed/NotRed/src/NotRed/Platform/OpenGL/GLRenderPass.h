#pragma once

#include "NotRed/Renderer/RenderPass.h"

namespace NR
{
	class GLRenderPass : public RenderPass
	{
	public:
		GLRenderPass(const RenderPassSpecification& spec);
		virtual ~GLRenderPass();

		const RenderPassSpecification& GetSpecification() const override { return mSpecification; }

	private:
		RenderPassSpecification mSpecification;
	};

}