#include "NRpch.h"
#include "VKRenderCommandBuffer.h"

#include "NotRed/Platform/Vulkan/VKContext.h"

namespace NR
{
	VKRenderCommandBuffer::VKRenderCommandBuffer(uint32_t count)
	{
		auto device = VKContext::GetCurrentDevice();

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
			count = Renderer::GetConfig().FramesInFlight;
		}
		commandBufferAllocateInfo.commandBufferCount = count;

		mCommandBuffers.resize(count);
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device->GetVulkanDevice(), &commandBufferAllocateInfo, mCommandBuffers.data()));
		
		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		mWaitFences.resize(Renderer::GetConfig().FramesInFlight);

		for (auto& fence : mWaitFences)
		{
			VK_CHECK_RESULT(vkCreateFence(device->GetVulkanDevice(), &fenceCreateInfo, nullptr, &fence));
		}
	}

	VKRenderCommandBuffer::~VKRenderCommandBuffer()
	{
		VkCommandPool commandPool = mCommandPool;
		Renderer::SubmitResourceFree([commandPool]()
			{
				auto device = VKContext::GetCurrentDevice();
				vkDestroyCommandPool(device->GetVulkanDevice(), commandPool, nullptr);
			});
	}

	void VKRenderCommandBuffer::Begin()
	{
		Ref<VKRenderCommandBuffer> instance = this;
		Renderer::Submit([instance]()
			{
				VkCommandBufferBeginInfo cmdBufInfo = {};
				cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				cmdBufInfo.pNext = nullptr;

				VK_CHECK_RESULT(vkBeginCommandBuffer(instance->mCommandBuffers[Renderer::GetCurrentFrameIndex()], &cmdBufInfo));
			});
	}

	void VKRenderCommandBuffer::End()
	{
		Ref<VKRenderCommandBuffer> instance = this;
		Renderer::Submit([instance]()
			{
				VK_CHECK_RESULT(vkEndCommandBuffer(instance->mCommandBuffers[Renderer::GetCurrentFrameIndex()]));
			});
	}

	void VKRenderCommandBuffer::Submit()
	{
		Ref<VKRenderCommandBuffer> instance = this;
		Renderer::Submit([instance]()
			{
				auto device = VKContext::GetCurrentDevice();
				uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
				
				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				
				VkCommandBuffer commandBuffer = instance->mCommandBuffers[Renderer::GetCurrentFrameIndex()];
				submitInfo.pCommandBuffers = &commandBuffer;

				VK_CHECK_RESULT(vkWaitForFences(device->GetVulkanDevice(), 1, &instance->mWaitFences[frameIndex], VK_TRUE, UINT64_MAX));
				VK_CHECK_RESULT(vkResetFences(device->GetVulkanDevice(), 1, &instance->mWaitFences[frameIndex]));
				VK_CHECK_RESULT(vkQueueSubmit(device->GetGraphicsQueue(), 1, &submitInfo, instance->mWaitFences[frameIndex]));
			});
	}
}