#pragma once

#include "NotRed/Renderer/Pipeline.h"

namespace NR
{
	class GLPipeline : public Pipeline
	{
	public:
		GLPipeline(const PipelineSpecification& spec);
		~GLPipeline() override;

		PipelineSpecification& GetSpecification() override { return mSpecification; }
		const PipelineSpecification& GetSpecification() const override { return mSpecification; }

		void Invalidate() override;

		void Bind() override;

	private:
		RendererID mID = 0;
		PipelineSpecification mSpecification;
	};
}