#pragma once

#include "NotRed/Core/Core.h"
#include "NotRed/Renderer/Shader.h"
#include "NotRed/Renderer/RenderCommandBuffer.h"

namespace NR
{
	class PipelineCompute : public RefCounted
	{
	public:
		virtual void Begin(Ref<RenderCommandBuffer> renderCommandBuffer = nullptr) = 0;
		virtual void End() = 0;

		virtual Ref<Shader> GetShader() = 0;

		static Ref<PipelineCompute> Create(Ref<Shader> computeShader);
	};
}