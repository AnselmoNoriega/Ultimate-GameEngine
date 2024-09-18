#pragma once

#include "Vulkan.h"

namespace NR::Utils 
{
	struct VKCheckpointData
	{
		char Data[64];
	};

	void SetVKCheckpoint(VkCommandBuffer commandBuffer, const std::string& data);
}