#pragma once

#include "NotRed/Renderer/Pipeline.h"

#include <map>

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
		void SetUniformBuffer(Ref<UniformBuffer> uniformBuffer, uint32_t binding, uint32_t set = 0) override;

		void Bind() override;

		VkPipeline GetVulkanPipeline() { return mVulkanPipeline; }
		VkPipelineLayout GetVulkanPipelineLayout() { return mPipelineLayout; }
		VkDescriptorSet GetDescriptorSet() { return mDescriptorSets.DescriptorSets[0]; }

		VkDescriptorSet GetDescriptorSet(uint32_t set = 0)
		{
			NR_CORE_ASSERT(mDescriptorSets.DescriptorSets.size() > set);
			return mDescriptorSets.DescriptorSets[set];
		}
		const std::vector<VkDescriptorSet>& GetDescriptorSets() const { return mDescriptorSets.DescriptorSets; }
		void RT_SetUniformBuffer(Ref<UniformBuffer> uniformBuffer, uint32_t binding, uint32_t set = 0);

	private:
		PipelineSpecification mSpecification;

		VkPipelineLayout mPipelineLayout;
		VkPipeline mVulkanPipeline;
		VKShader::ShaderMaterialDescriptorSet mDescriptorSets;
	};
}