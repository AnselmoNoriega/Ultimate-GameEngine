#pragma once

#include <string>
#include "Vulkan.h"
#include "VKDevice.h"

namespace NR
{
	class VKAllocator
	{
	public:
		VKAllocator() = default;
		VKAllocator(const std::string& tag);
		VKAllocator(const Ref<VKDevice>& device, const std::string& tag = "");
		~VKAllocator();

		void Allocate(VkMemoryRequirements requirements, VkDeviceMemory* dest, VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
	private:
		Ref<VKDevice> mDevice;
		std::string mTag;
	};
}