#pragma once

#include <vector>

#include "NotRed/Core/Core.h"
#include "NotRed/Renderer/RenderCommandBuffer.h"

#include "Vulkan.h"
#include "VKDevice.h"
#include "VKAllocator.h"

struct GLFWwindow;

namespace NR
{
	class VKSwapChain
	{
	public:
		VKSwapChain() = default;

		void Init(VkInstance instance, const Ref<VKDevice>& device);
		void InitSurface(GLFWwindow* windowHandle);
		void Create(uint32_t* width, uint32_t* height, bool vsync);

		void Resize(uint32_t width, uint32_t height);

		void BeginFrame();
		void Present();

		uint32_t GetImageCount() const { return mImageCount; }

		uint32_t GetWidth() const { return mWidth; }
		uint32_t GetHeight() const { return mHeight; }

		VkRenderPass GetRenderPass() { return mRenderPass; }

		VkFramebuffer GetCurrentFrameBuffer() { return GetFrameBuffer(mCurrentImageIndex); }
		VkCommandBuffer GetCurrentDrawCommandBuffer() { return GetDrawCommandBuffer(mCurrentBufferIndex); }

		VkFormat GetColorFormat() { return mColorFormat; }

		uint32_t GetCurrentBufferIndex() const { return mCurrentBufferIndex; }
		VkFramebuffer GetFrameBuffer(uint32_t index)
		{
			NR_CORE_ASSERT(index < mFrameBuffers.size());
			return mFrameBuffers[index];
		}

		VkCommandBuffer GetDrawCommandBuffer(uint32_t index)
		{
			NR_CORE_ASSERT(index < mCommandBuffers.size());
			return mCommandBuffers[index];
		}

		VkSemaphore GetRenderCompleteSemaphore() { return mSemaphores.RenderComplete; }

		void Cleanup();

	private:
		VkResult AcquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex);
		VkResult QueuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE);

		void CreateFrameBuffer();
		void CreateDepthStencil();
		void FindImageFormatAndColorSpace();

	private:
		struct SwapChainBuffer
		{
			VkImage image;
			VkImageView view;
		};

	private:
		VkInstance mInstance;
		Ref<VKDevice> mDevice;
		VKAllocator mAllocator;

		bool mVSync = false;

		VkFormat mColorFormat;
		VkColorSpaceKHR mColorSpace;

		VkSwapchainKHR mSwapChain = nullptr;
		uint32_t mImageCount = 0;
		std::vector<VkImage> mImages;

		std::vector<SwapChainBuffer> mBuffers;

		struct
		{
			VkImage Image;
			VmaAllocation MemoryAlloc;
			VkImageView ImageView;
		} mDepthStencil;

		struct
		{
			// Swap chain
			VkSemaphore PresentComplete;
			// Command buffer
			VkSemaphore RenderComplete;
		} mSemaphores;

		std::vector<VkFramebuffer> mFrameBuffers;
		VkCommandPool mCommandPool = nullptr;
		std::vector<VkCommandBuffer> mCommandBuffers;

		VkSubmitInfo mSubmitInfo;

		std::vector<VkFence> mWaitFences;

		VkRenderPass mRenderPass;
		uint32_t mCurrentBufferIndex = 0;
		uint32_t mCurrentImageIndex = 0;

		uint32_t mQueueNodeIndex = UINT32_MAX;
		uint32_t mWidth = 0, mHeight = 0;

		VkSurfaceKHR mSurface;

		friend class VKContext;
	};
}