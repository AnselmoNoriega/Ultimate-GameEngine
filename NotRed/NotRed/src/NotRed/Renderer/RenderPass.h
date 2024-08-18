#pragma once

#include "NotRed/Core/Core.h"

#include "Framebuffer.h"

namespace NR
{
	struct RenderPassSpecification
	{
		Ref<FrameBuffer> TargetFrameBuffer;
	};

	class RenderPass
	{
	public:
		virtual ~RenderPass() {}

		virtual const RenderPassSpecification& GetSpecification() const = 0;

		static Ref<RenderPass> Create(const RenderPassSpecification& spec);
	};
}