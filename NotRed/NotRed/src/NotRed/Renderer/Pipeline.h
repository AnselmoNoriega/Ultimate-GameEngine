#pragma once

#include "NotRed/Core/Ref.h"

#include "NotRed/Renderer/VertexBuffer.h"
#include "NotRed/Renderer/Shader.h"

namespace NR
{
	struct PipelineSpecification
	{
		Ref<NR::Shader> Shader;
		VertexBufferLayout Layout;
	};

	class Pipeline : public RefCounted
	{
	public:
		virtual ~Pipeline() = default;

		virtual PipelineSpecification& GetSpecification() = 0;
		virtual const PipelineSpecification& GetSpecification() const = 0;

		virtual void Invalidate() = 0;

		virtual void Bind() = 0;

		static Ref<Pipeline> Create(const PipelineSpecification& spec);
	};
}