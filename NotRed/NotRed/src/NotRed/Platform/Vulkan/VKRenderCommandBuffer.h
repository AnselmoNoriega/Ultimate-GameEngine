#pragma once

#include "NotRed/Renderer/RenderCommandBuffer.h"

#include "Vulkan/vulkan.h"

namespace NR
{
	class VKRenderCommandBuffer : public RenderCommandBuffer
	{
	public:
		VKRenderCommandBuffer(uint32_t count = 0);
		~VKRenderCommandBuffer();

		void Begin() override;
		void End() override;
		void Submit() override;

		VkCommandBuffer GetCommandBuffer(uint32_t frameIndex) const
		{
			NR_CORE_ASSERT(frameIndex < mCommandBuffers.size());
			return mCommandBuffers[frameIndex];
		}

	private:
		VkCommandPool mCommandPool = nullptr;

		std::vector<VkCommandBuffer> mCommandBuffers;
		std::vector<VkFence> mWaitFences;

		int mActiveBufferIndex = -1;
	};
}