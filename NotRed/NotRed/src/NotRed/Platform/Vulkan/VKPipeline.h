#pragma once

#include "NotRed/Renderer/Pipeline.h"

#include "Vulkan.h"
#include "VKShader.h"

namespace NR
{
	class VKPipeline : public Pipeline
	{
	public:
		VKPipeline(const PipelineSpecification& spec);
		~VKPipeline() override;

		PipelineSpecification& GetSpecification() override { return mSpecification; }
		const PipelineSpecification& GetSpecification() const override { return mSpecification; }

		void Invalidate() override;

		void Bind() override;

		VkPipeline GetVulkanPipeline() { return mVulkanPipeline; }
		VkPipelineLayout GetVulkanPipelineLayout() { return mPipelineLayout; }
		VkDescriptorSet GetDescriptorSet() { return mDescriptorSet.DescriptorSets[0]; }

	private:
		PipelineSpecification mSpecification;

		VkPipelineLayout mPipelineLayout;
		VkPipeline mVulkanPipeline;
		VKShader::ShaderMaterialDescriptorSet mDescriptorSet;
	};
}