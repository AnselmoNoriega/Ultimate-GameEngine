#include "nrpch.h"
#include "VKAllocator.h"

#include "VKContext.h"

namespace NR
{
	VKAllocator::VKAllocator(const Ref<VKDevice>& device, const std::string& tag)
		: mDevice(device), mTag(tag)
	{
	}

	VKAllocator::VKAllocator(const std::string& tag)
		: mDevice(VKContext::GetCurrentDevice()), mTag(tag)
	{
	}

	VKAllocator::~VKAllocator()
	{
	}

	void VKAllocator::Allocate(VkMemoryRequirements requirements, VkDeviceMemory* dest, VkMemoryPropertyFlags flags /*= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT*/)
	{
		NR_CORE_ASSERT(mDevice);

		NR_CORE_TRACE("VKAllocator ({0}): allocating {1} bytes", mTag, requirements.size);

		VkMemoryAllocateInfo memAlloc = {};
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAlloc.allocationSize = requirements.size;
		memAlloc.memoryTypeIndex = mDevice->GetPhysicalDevice()->GetMemoryTypeIndex(requirements.memoryTypeBits, flags);
		VK_CHECK_RESULT(vkAllocateMemory(mDevice->GetVulkanDevice(), &memAlloc, nullptr, dest));
	}

}