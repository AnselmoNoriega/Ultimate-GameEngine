#pragma once

#include <string>
#include "Vulkan.h"
#include "VKDevice.h"
#include "VulkanMemoryAllocator/vk_mem_alloc.h"

namespace NR
{
	struct GPUMemoryStats
	{
		uint64_t Used = 0;
		uint64_t Free = 0;
	};

	class VKAllocator
	{
	public:
		VKAllocator() = default;
		VKAllocator(const std::string& tag);
		~VKAllocator();

		VmaAllocation AllocateBuffer(VkBufferCreateInfo bufferCreateInfo, VmaMemoryUsage usage, VkBuffer& outBuffer);
		VmaAllocation AllocateImage(VkImageCreateInfo imageCreateInfo, VmaMemoryUsage usage, VkImage& outImage);
		void Free(VmaAllocation allocation);
		void DestroyImage(VkImage image, VmaAllocation allocation);
		void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation);

		template<typename T>
		T* MapMemory(VmaAllocation allocation)
		{
			T* mappedMemory;
			VkResult result = vmaMapMemory(VKAllocator::GetVMAAllocator(), allocation, (void**)&mappedMemory); 
			if (result != VK_SUCCESS) 
			{
				NR_CORE_ASSERT(false, "Upsy!");
			}
			return mappedMemory;
		}

		void UnmapMemory(VmaAllocation allocation);

		static void DumpStats();
		static GPUMemoryStats GetStats();

		static void Init(Ref<VKDevice> device);
		static void Shutdown();

		static VmaAllocator& GetVMAAllocator();
	
	private:
		std::string mTag;
	};
}