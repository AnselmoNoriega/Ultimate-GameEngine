#pragma once

#include "NotRed/Renderer/PipelineCompute.h"
#include "VKShader.h"

#include "vulkan/vulkan.h"
#include "VKRenderCommandBuffer.h"

namespace NR
{
	class VKComputePipeline : public PipelineCompute
	{
	public:
		VKComputePipeline(Ref<Shader> computeShader);

		void Execute(VkDescriptorSet* descriptorSets, uint32_t descriptorSetCount, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

		void Begin(Ref<RenderCommandBuffer> renderCommandBuffer = nullptr) override;
		void Dispatch(VkDescriptorSet descriptorSet, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
		void End() override;
		
		void CreatePipeline();

		Ref<Shader> GetShader() override { return mShader; }
		VkCommandBuffer GetActiveCommandBuffer() { return mActiveComputeCommandBuffer; }

		void SetPushConstants(const void* data, uint32_t size);

		VkPipeline GetVulkanPipeline() { return mComputePipeline; }
		VkPipelineLayout GetVulkanPipelineLayout() { return mComputePipelineLayout; }

	private:
		void RT_CreatePipeline();

	private:
		Ref<VKShader> mShader;

		VkPipelineLayout mComputePipelineLayout = nullptr;
		VkPipelineCache mPipelineCache = nullptr;
		VkPipeline mComputePipeline = nullptr;

		VkCommandBuffer mActiveComputeCommandBuffer = nullptr;
		
		bool mUsingGraphicsQueue = false;
	};
}