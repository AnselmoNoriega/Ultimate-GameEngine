#include "nrpch.h"
#include "AudioFileUtils.h"

#include "miniaudio.h"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#include "stb_vorbis.c"

#include "NotRed/Asset/AssetManager.h"
#include "NotRed/Util/FileSystem.h"
#include "NotRed/Util/StringUtils.h"

#if NR_PLATFORM_WINDOWS
int64_t GetFileSizeWindows(const char* name)
{
	WIN32_FILE_ATTRIBUTE_DATA fad;
	if (!GetFileAttributesExA(name, GetFileExInfoStandard, &fad))
	{
		return -1; // error condition, could call GetLastError to find out more
	}

	LARGE_INTEGER size;
	size.HighPart = fad.nFileSizeHigh;
	size.LowPart = fad.nFileSizeLow;

	return size.QuadPart;
}
#endif

namespace NR::AudioFileUtils
{
	static std::optional<AudioFileInfo> GetFileInfoWav(const char* filepath)
	{
		drwav wav;
		if (drwav_init_file(&wav, filepath, nullptr))
		{
			ma_uint16 bitDepth = wav.bitsPerSample;
			ma_uint16 channels = wav.channels;
			ma_uint32 sampleRate = wav.sampleRate;

			double duration = (double)wav.totalPCMFrameCount / (double)sampleRate;

			auto dataSize = wav.dataChunkDataSize;
			auto pos = wav.dataChunkDataPos;
			auto fileSize = dataSize + pos;

			drwav_uninit(&wav);

			return AudioFileInfo{ duration, sampleRate, bitDepth, channels, fileSize };
		}

		return std::optional<AudioFileInfo>();
	}

	static std::optional<AudioFileInfo> GetFileInfoVorbis(const char* filepath)
	{
		STBVorbisError error;
		if (stb_vorbis* vorbis = stb_vorbis_open_filename(filepath, (int*)&error, nullptr))
		{
			ma_uint16 bitDepth = 0;
			ma_uint16 channels = vorbis->channels;
			ma_uint32 sampleRate = vorbis->sample_rate;

			double duration = (double)vorbis->total_samples / (double)sampleRate;

			uint64_t fileSize;
#if NR_PLATFORM_WINDOWS
			fileSize = GetFileSizeWindows(filepath);
#endif
			stb_vorbis_close(vorbis);
			return AudioFileInfo{ duration, sampleRate, bitDepth, channels, fileSize };
		}

		return std::optional<AudioFileInfo>();
	}

	std::optional<AudioFileInfo> GetFileInfo(const AssetMetadata& metadata)
	{
		std::string filepath = AssetManager::GetFileSystemPath(metadata);
		if (Utils::String::EqualsIgnoreCase(metadata.FilePath.extension().string(), ".wav"))
		{
			return GetFileInfoWav(filepath.c_str());
		}
		else if (Utils::String::EqualsIgnoreCase(metadata.FilePath.extension().string(), ".ogg"))
		{
			return GetFileInfoVorbis(filepath.c_str());
		}
		else
		{
			return std::optional<AudioFileInfo>();
		}
	}

	std::optional<AudioFileInfo> GetFileInfo(const char* filepath)
	{
		AssetMetadata metadata;
		metadata.FilePath = filepath;
		metadata.IsDataLoaded = false;
		return GetFileInfo(metadata);
	}

	std::string ChannelsToLayoutString(uint16_t numChannels)
	{
		std::string str;
		switch (numChannels)
		{
		case 1: str = "Mono"; break;
		case 2: str = "Stereo"; break;
		case 3: str = "3.0"; break;
		case 4: str = "Quad"; break;
		case 5: str = "5.0"; break;
		case 6: str = "5.1"; break;
		//case 7: str = "Unknown"; break;
		case 8: str = "7.1"; break;
		default: str = "Unknown layout"; break;
		}
		return str;
	}
}