#pragma once

#include <optional>

#include "NotRed/Asset/Asset.h"
#include "NotRed/Asset/AssetMetadata.h"

namespace NR::AudioFileUtils
{
	struct AudioFileInfo
	{
		double Duration;
		uint32_t SamplingRate;
		uint16_t BidDepth;
		uint16_t NumChannels;
		uint64_t FileSize;
	};

	std::optional<AudioFileInfo> GetFileInfo(const AssetMetadata& metadata);
	std::optional<AudioFileInfo> GetFileInfo(const char* filepath);
}