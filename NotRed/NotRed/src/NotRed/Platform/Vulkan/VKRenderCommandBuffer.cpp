#include "NRpch.h"
#include "VKRenderCommandBuffer.h"

#include <utility>

#include "NotRed/Platform/Vulkan/VKContext.h"

namespace NR
{
	VKRenderCommandBuffer::VKRenderCommandBuffer(uint32_t count, std::string debugName)
		: mDebugName(std::move(debugName))
	{
		auto device = VKContext::GetCurrentDevice();
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = device->GetPhysicalDevice()->GetQueueFamilyIndices().Graphics;
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		
		VK_CHECK_RESULT(vkCreateCommandPool(device->GetVulkanDevice(), &cmdPoolInfo, nullptr, &mCommandPool));
		
		VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandPool = mCommandPool;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		if (count == 0)
		{
			count = framesInFlight;
		}
		commandBufferAllocateInfo.commandBufferCount = count;

		mCommandBuffers.resize(count);
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device->GetVulkanDevice(), &commandBufferAllocateInfo, mCommandBuffers.data()));
		
		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		mWaitFences.resize(framesInFlight);

		for (auto& fence : mWaitFences)
		{
			VK_CHECK_RESULT(vkCreateFence(device->GetVulkanDevice(), &fenceCreateInfo, nullptr, &fence));
		}

		VkQueryPoolCreateInfo queryPoolCreateInfo = {};
		queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		queryPoolCreateInfo.pNext = nullptr;
		
		// Timestamp queries
		const uint32_t maxUserQueries = 10;
		mTimestampQueryCount = 2 + 2 * maxUserQueries;
		queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		queryPoolCreateInfo.queryCount = mTimestampQueryCount;
		mTimestampQueryPools.resize(framesInFlight);

		for (auto& timestampQueryPool : mTimestampQueryPools)
		{
			VK_CHECK_RESULT(vkCreateQueryPool(device->GetVulkanDevice(), &queryPoolCreateInfo, nullptr, &timestampQueryPool));
		}

		mTimestampQueryResults.resize(framesInFlight);
		
		for (auto& timestampQueryResults : mTimestampQueryResults)
		{
			timestampQueryResults.resize(mTimestampQueryCount);
		}

		mExecutionGPUTimes.resize(framesInFlight);
		
		for (auto& executionGPUTimes : mExecutionGPUTimes)
		{
			executionGPUTimes.resize(mTimestampQueryCount / 2);
		}

		// Pipeline statistics queries
		mPipelineQueryCount = 7;
		queryPoolCreateInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
		queryPoolCreateInfo.queryCount = mPipelineQueryCount;
		queryPoolCreateInfo.pipelineStatistics =
			VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;

		mPipelineStatisticsQueryPools.resize(framesInFlight);

		for (auto& pipelineStatisticsQueryPools : mPipelineStatisticsQueryPools)
		{
			VK_CHECK_RESULT(vkCreateQueryPool(device->GetVulkanDevice(), &queryPoolCreateInfo, nullptr, &pipelineStatisticsQueryPools));
		}

		mPipelineStatisticsQueryResults.resize(framesInFlight);
	}

	VKRenderCommandBuffer::VKRenderCommandBuffer(std::string debugName, bool swapchain)
		: mDebugName(std::move(debugName)), mOwnedBySwapChain(true)
	{
		auto device = VKContext::GetCurrentDevice();
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		mCommandBuffers.resize(framesInFlight);

		VKSwapChain& swapChain = Application::Get().GetWindow().GetSwapChain();
		for (uint32_t frame = 0; frame < framesInFlight; ++frame)
		{
			mCommandBuffers[frame] = swapChain.GetDrawCommandBuffer(frame);
		}

		VkQueryPoolCreateInfo queryPoolCreateInfo = {};
		queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		queryPoolCreateInfo.pNext = nullptr;
		
		// Timestamp queries
		const uint32_t maxUserQueries = 10;
		mTimestampQueryCount = 2 + 2 * maxUserQueries;

		queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		queryPoolCreateInfo.queryCount = mTimestampQueryCount;
		mTimestampQueryPools.resize(framesInFlight);
		
		for (auto& timestampQueryPool : mTimestampQueryPools)
		{
			VK_CHECK_RESULT(vkCreateQueryPool(device->GetVulkanDevice(), &queryPoolCreateInfo, nullptr, &timestampQueryPool));
		}

		mTimestampQueryResults.resize(framesInFlight);
		
		for (auto& timestampQueryResults : mTimestampQueryResults)
		{
			timestampQueryResults.resize(mTimestampQueryCount);
		}
		
		mExecutionGPUTimes.resize(framesInFlight);
		
		for (auto& executionGPUTimes : mExecutionGPUTimes)
		{
			executionGPUTimes.resize(mTimestampQueryCount / 2);
		}

		// Pipeline statistics queries
		mPipelineQueryCount = 7;
		queryPoolCreateInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
		queryPoolCreateInfo.queryCount = mPipelineQueryCount;
		queryPoolCreateInfo.pipelineStatistics =
			VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;
		
		mPipelineStatisticsQueryPools.resize(framesInFlight);
		
		for (auto& pipelineStatisticsQueryPools : mPipelineStatisticsQueryPools)
		{
			VK_CHECK_RESULT(vkCreateQueryPool(device->GetVulkanDevice(), &queryPoolCreateInfo, nullptr, &pipelineStatisticsQueryPools));
		}

		mPipelineStatisticsQueryResults.resize(framesInFlight);
	}

	VKRenderCommandBuffer::~VKRenderCommandBuffer()
	{
		if (mOwnedBySwapChain)
		{
			return;
		}

		VkCommandPool commandPool = mCommandPool;
		Renderer::SubmitResourceFree([commandPool]()
			{
				auto device = VKContext::GetCurrentDevice();
				vkDestroyCommandPool(device->GetVulkanDevice(), commandPool, nullptr);
			});
	}

	void VKRenderCommandBuffer::Begin()
	{
		mTimestampNextAvailableQuery = 2;

		Ref<VKRenderCommandBuffer> instance = this;
		Renderer::Submit([instance]() mutable
			{
				uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

				VkCommandBufferBeginInfo cmdBufInfo = {};
				cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				cmdBufInfo.pNext = nullptr;

				VkCommandBuffer commandBuffer = instance->mCommandBuffers[frameIndex];
				VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));
				
				// Timestamp query
				vkCmdResetQueryPool(commandBuffer, instance->mTimestampQueryPools[frameIndex], 0, instance->mTimestampQueryCount);
				vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, instance->mTimestampQueryPools[frameIndex], 0);
				
				// Pipeline stats query
				vkCmdResetQueryPool(commandBuffer, instance->mPipelineStatisticsQueryPools[frameIndex], 0, instance->mPipelineQueryCount);
				vkCmdBeginQuery(commandBuffer, instance->mPipelineStatisticsQueryPools[frameIndex], 0, 0);
			});
	}

	void VKRenderCommandBuffer::End()
	{
		Ref<VKRenderCommandBuffer> instance = this;
		Renderer::Submit([instance]()
			{
				uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
				VkCommandBuffer commandBuffer = instance->mCommandBuffers[frameIndex];
				vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, instance->mTimestampQueryPools[frameIndex], 1);
				vkCmdEndQuery(commandBuffer, instance->mPipelineStatisticsQueryPools[frameIndex], 0);

				VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
			});
	}

	void VKRenderCommandBuffer::Submit()
	{
		if (mOwnedBySwapChain)
		{
			return;
		}

		Ref<VKRenderCommandBuffer> instance = this;
		Renderer::Submit([instance]() mutable
			{
				auto device = VKContext::GetCurrentDevice();
				uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
				
				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;

				VkCommandBuffer commandBuffer = instance->mCommandBuffers[frameIndex];
				submitInfo.pCommandBuffers = &commandBuffer;

				VK_CHECK_RESULT(vkWaitForFences(device->GetVulkanDevice(), 1, &instance->mWaitFences[frameIndex], VK_TRUE, UINT64_MAX));
				VK_CHECK_RESULT(vkResetFences(device->GetVulkanDevice(), 1, &instance->mWaitFences[frameIndex]));
				VK_CHECK_RESULT(vkQueueSubmit(device->GetGraphicsQueue(), 1, &submitInfo, instance->mWaitFences[frameIndex]));
				// Retrieve timestamp query results
				vkGetQueryPoolResults(device->GetVulkanDevice(), instance->mTimestampQueryPools[frameIndex], 0, instance->mTimestampNextAvailableQuery,
					instance->mTimestampNextAvailableQuery * sizeof(uint64_t), instance->mTimestampQueryResults[frameIndex].data(), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);

				for (uint32_t i = 0; i < instance->mTimestampNextAvailableQuery; i += 2)
				{
					uint64_t startTime = instance->mTimestampQueryResults[frameIndex][i];
					uint64_t endTime = instance->mTimestampQueryResults[frameIndex][i + 1];
					float nsTime = endTime > startTime ? (endTime - startTime) * device->GetPhysicalDevice()->GetLimits().timestampPeriod : 0.0f;
				
					instance->mExecutionGPUTimes[frameIndex][i / 2] = nsTime * 0.000001f; // Time in ms
				}

				// Retrieve pipeline stats results
				vkGetQueryPoolResults(device->GetVulkanDevice(), instance->mPipelineStatisticsQueryPools[frameIndex], 0, 1,
					sizeof(PipelineStatistics), &instance->mPipelineStatisticsQueryResults[frameIndex], sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
			});
	}

	uint64_t VKRenderCommandBuffer::BeginTimestampQuery()
	{
		uint64_t queryIndex = mTimestampNextAvailableQuery;
		mTimestampNextAvailableQuery += 2;
		Ref<VKRenderCommandBuffer> instance = this;

		Renderer::Submit([instance, queryIndex]()
			{
				uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
				VkCommandBuffer commandBuffer = instance->mCommandBuffers[frameIndex];
				vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, instance->mTimestampQueryPools[frameIndex], queryIndex);
			});

		return queryIndex;
	}

	void VKRenderCommandBuffer::EndTimestampQuery(uint64_t queryID)
	{
		Ref<VKRenderCommandBuffer> instance = this;

		Renderer::Submit([instance, queryID]()
			{
				uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
				VkCommandBuffer commandBuffer = instance->mCommandBuffers[frameIndex];
				vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, instance->mTimestampQueryPools[frameIndex], queryID + 1);
			});
	}
}