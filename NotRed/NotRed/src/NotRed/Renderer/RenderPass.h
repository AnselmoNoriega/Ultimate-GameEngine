#pragma once

#include "NotRed/Core/Core.h"

#include "Framebuffer.h"

namespace NR
{
	struct RenderPassSpecification
	{
		Ref<FrameBuffer> TargetFrameBuffer;
	};

	class RenderPass : public RefCounted
	{
	public:
		virtual ~RenderPass() = default;

		virtual RenderPassSpecification& GetSpecification() = 0;

		virtual const RenderPassSpecification& GetSpecification() const = 0;

		static Ref<RenderPass> Create(const RenderPassSpecification& spec);
	};
}