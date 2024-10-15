#pragma once

#include "VKShader.h"

#include "vulkan/vulkan.h"
#include "VKRenderCommandBuffer.h"

namespace NR
{
	class VKComputePipeline : public RefCounted
	{
	public:
		VKComputePipeline(Ref<Shader> computeShader);
		~VKComputePipeline();

		void Execute(VkDescriptorSet* descriptorSets, uint32_t descriptorSetCount, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

		void Begin(Ref<RenderCommandBuffer> renderCommandBuffer = nullptr);
		void Dispatch(VkDescriptorSet descriptorSet, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
		void End();

		Ref<VKShader> GetShader() { return mShader; }

		void SetPushConstants(const void* data, uint32_t size);

		VkPipeline GetVulkanPipeline() { return mComputePipeline; }
		VkPipelineLayout GetVulkanPipelineLayout() { return mComputePipelineLayout; }

	private:
		void CreatePipeline();

	private:
		Ref<VKShader> mShader;

		VkPipelineLayout mComputePipelineLayout = nullptr;
		VkPipelineCache mPipelineCache = nullptr;
		VkPipeline mComputePipeline = nullptr;

		VkCommandBuffer mActiveComputeCommandBuffer = nullptr;
		
		bool mUsingGraphicsQueue = false;
	};
}