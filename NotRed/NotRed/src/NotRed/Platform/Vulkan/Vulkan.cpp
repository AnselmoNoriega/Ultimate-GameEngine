#include "nrpch.h"
#include "Vulkan.h"

#include "VKContext.h"
#include "VKDiagnostics.h"

namespace NR::Utils 
{
	static const char* StageToString(VkPipelineStageFlagBits stage)
	{
		switch (stage)
		{
		case VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT: return "VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT";
		case VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT: return "VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT";
		default:
			NR_CORE_ASSERT(false);
			return nullptr;
		}
	}

	void RetrieveDiagnosticCheckpoints()
	{
		{
			const uint32_t checkpointCount = 4;
			VkCheckpointDataNV data[checkpointCount];
			for (uint32_t i = 0; i < checkpointCount; ++i)
			{
				data[i].sType = VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV;
			}

			uint32_t retrievedCount = checkpointCount;
			vkGetQueueCheckpointDataNV(::NR::VKContext::GetCurrentDevice()->GetQueue(), &retrievedCount, data);
			NR_CORE_ERROR("RetrieveDiagnosticCheckpoints (Graphics Queue):");
			for (uint32_t i = 0; i < retrievedCount; ++i)
			{
				VKCheckpointData* checkpoint = (VKCheckpointData*)data[i].pCheckpointMarker;
				NR_CORE_ERROR("Checkpoint: {0} (stage: {1})", checkpoint->Data, StageToString(data[i].stage));
			}
		}
		{
			const uint32_t checkpointCount = 4;
			VkCheckpointDataNV data[checkpointCount];
			for (uint32_t i = 0; i < checkpointCount; ++i)
			{
				data[i].sType = VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV;
			}

			uint32_t retrievedCount = checkpointCount;
			vkGetQueueCheckpointDataNV(::NR::VKContext::GetCurrentDevice()->GetComputeQueue(), &retrievedCount, data);
			NR_CORE_ERROR("RetrieveDiagnosticCheckpoints (Compute Queue):");
			for (uint32_t i = 0; i < retrievedCount; ++i)
			{
				VKCheckpointData* checkpoint = (VKCheckpointData*)data[i].pCheckpointMarker;
				NR_CORE_ERROR("Checkpoint: {0} (stage: {1})", checkpoint->Data, StageToString(data[i].stage));
			}
		}
		__debugbreak();
	}
}