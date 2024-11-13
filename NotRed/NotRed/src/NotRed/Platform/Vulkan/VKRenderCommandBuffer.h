#pragma once

#include "NotRed/Renderer/RenderCommandBuffer.h"

#include "Vulkan/vulkan.h"

namespace NR
{
	class VKRenderCommandBuffer : public RenderCommandBuffer
	{
	public:
		VKRenderCommandBuffer(uint32_t count = 0, std::string debugName = "");
		VKRenderCommandBuffer(std::string debugName, bool swapchain);
		~VKRenderCommandBuffer() override;

		void Begin() override;
		void End() override;
		void Submit() override;

		float GetExecutionGPUTime(uint32_t frameIndex, uint32_t queryIndex = 0) const override
		{
			if (queryIndex / 2 >= mTimestampNextAvailableQuery / 2)
			{
				return 0.0f;
			}

			return mExecutionGPUTimes[frameIndex][queryIndex / 2];
		}

		const PipelineStatistics& GetPipelineStatistics(uint32_t frameIndex) const { return mPipelineStatisticsQueryResults[frameIndex]; }
		uint64_t BeginTimestampQuery() override;
		void EndTimestampQuery(uint64_t queryID) override;

		VkCommandBuffer GetCommandBuffer(uint32_t frameIndex) const
		{
			NR_CORE_ASSERT(frameIndex < mCommandBuffers.size());
			return mCommandBuffers[frameIndex];
		}

	private:
		std::string mDebugName;

		VkCommandPool mCommandPool = nullptr;

		std::vector<VkCommandBuffer> mCommandBuffers;
		std::vector<VkFence> mWaitFences;

		bool mOwnedBySwapChain = false;

		uint32_t mTimestampQueryCount = 0;
		uint32_t mTimestampNextAvailableQuery = 2;
		
		std::vector<VkQueryPool> mTimestampQueryPools;
		std::vector<VkQueryPool> mPipelineStatisticsQueryPools;
		std::vector<std::vector<uint64_t>> mTimestampQueryResults;
		std::vector<std::vector<float>> mExecutionGPUTimes;
		
		uint32_t mPipelineQueryCount = 0;

		std::vector<PipelineStatistics> mPipelineStatisticsQueryResults;
	};
}