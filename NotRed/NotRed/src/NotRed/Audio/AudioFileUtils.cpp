#include "nrpch.h"
#include "AudioFileUtils.h"

#include "miniaudio.h"
#include "dr_wav.h"
#include "stb_vorbis.c"

#include "NotRed/Asset/AssetManager.h"
#include "NotRed/Util/FileSystem.h"
#include "NotRed/Util/StringUtils.h"

std::string BytesToString(uint64_t bytes)
{
	static const float gb = 1024 * 1024 * 1024;
	static const float mb = 1024 * 1024;
	static const float kb = 1024;

	char buffer[16];

	if (bytes > gb)
	{
		sprintf_s(buffer, "%.2f GB", bytes / gb);
	}
	else if (bytes > mb)
	{
		sprintf_s(buffer, "%.2f MB", bytes / mb);
	}
	else if (bytes > kb)
	{
		sprintf_s(buffer, "%.2f KB", bytes / kb);
	}
	else
	{
		sprintf_s(buffer, "%.2f bytes", bytes);
	}
	return std::string(buffer);
}

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
			auto sizestr = BytesToString(fileSize);
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

			auto sizestr = BytesToString(fileSize);
			stb_vorbis_close(vorbis);
			return AudioFileInfo{ duration, sampleRate, bitDepth, channels, fileSize };
		}
		return std::optional<AudioFileInfo>();
	}

	std::optional<AudioFileInfo> GetFileInfo(const AssetMetadata& metadata)
	{
		std::string filepath = metadata.FilePath.string();
		if (metadata.Extension == "wav")
		{
			return GetFileInfoWav(filepath.c_str());
		}
		else if (metadata.Extension == "ogg")
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
		metadata.FileName = Utils::RemoveExtension(metadata.FilePath.filename().string());
		metadata.Extension = metadata.FilePath.extension().string();
		metadata.IsDataLoaded = false;

		return GetFileInfo(metadata);
	}
}