#pragma once

#include <unordered_set>

#include "NotRed/Core/Ref.h"

#include "Vulkan.h"

namespace NR
{
	class VKPhysicalDevice : public RefCounted
	{
	public:
		struct QueueFamilyIndices
		{
			int32_t Graphics = -1;
			int32_t Compute = -1;
			int32_t Transfer = -1;
		};

	public:
		VKPhysicalDevice();
		~VKPhysicalDevice();

		bool IsExtensionSupported(const std::string& extensionName) const;
		uint32_t GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const;

		VkPhysicalDevice GetVulkanPhysicalDevice() const { return mPhysicalDevice; }
		const QueueFamilyIndices& GetQueueFamilyIndices() const { return mQueueFamilyIndices; }

		const VkPhysicalDeviceProperties& GetProperties() const { return mProperties; }
		const VkPhysicalDeviceLimits& GetLimits() const { return mProperties.limits; }		
		const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const { return mMemoryProperties; }

		VkFormat GetDepthFormat() const { return mDepthFormat; }

		static Ref<VKPhysicalDevice> Select();

	private:
		VkFormat FindDepthFormat() const;
		QueueFamilyIndices GetQueueFamilyIndices(int queueFlags);

	private:
		QueueFamilyIndices mQueueFamilyIndices;

		VkPhysicalDevice mPhysicalDevice = nullptr;
		VkPhysicalDeviceFeatures mFeatures;
		VkPhysicalDeviceProperties mProperties;
		VkPhysicalDeviceMemoryProperties mMemoryProperties;

		VkFormat mDepthFormat = VK_FORMAT_UNDEFINED;

		std::vector<VkQueueFamilyProperties> mQueueFamilyProperties;
		std::unordered_set<std::string> mSupportedExtensions;
		std::vector<VkDeviceQueueCreateInfo> mQueueCreateInfos;

		friend class VKDevice;
	};

	// Represents a logical device
	class VKDevice : public RefCounted
	{
	public:
		VKDevice(const Ref<VKPhysicalDevice>& physicalDevice, VkPhysicalDeviceFeatures enabledFeatures);
		~VKDevice();

		void Destroy();

		VkQueue GetGraphicsQueue() { return mGraphicsQueue; }
		VkQueue GetComputeQueue() { return mComputeQueue; }

		VkCommandBuffer GetCommandBuffer(bool begin, bool compute = false);
		void FlushCommandBuffer(VkCommandBuffer commandBuffer);
		void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue);

		VkCommandBuffer CreateSecondaryCommandBuffer();

		const Ref<VKPhysicalDevice>& GetPhysicalDevice() const { return mPhysicalDevice; }
		VkDevice GetVulkanDevice() const { return mLogicalDevice; }
		
		VkCommandPool GetCommandPool() const { return mCommandPool; }

	private:
		Ref<VKPhysicalDevice> mPhysicalDevice;
		VkDevice mLogicalDevice = nullptr;

		VkPhysicalDeviceFeatures mEnabledFeatures;
		VkCommandPool mCommandPool, mComputeCommandPool;

		VkQueue mGraphicsQueue;
		VkQueue mComputeQueue;

		bool mEnableDebugMarkers = false;
	};
}