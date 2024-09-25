#pragma once
#include "Sound.h"

namespace NR::Audio
{
    class AudioPlayback
    {
    public:
        static bool PlaySound2D(const SoundConfig& soundConfig, float volume = 1.0f, float pitch = 1.0f);
        static bool PlaySound2D(Ref<Asset> soundAsset, float volume = 1.0f, float pitch = 1.0f);

        static bool PlaySoundAtLocation(const SoundConfig& soundConfig, glm::vec3 location, float volume = 1.0f, float pitch = 1.0f);     
        static bool PlaySoundAtLocation(Ref<Asset> soundAsset, glm::vec3 location, float volume = 1.0f, float pitch = 1.0f);

        static bool Play(uint64_t audioComponentID, float startTime = 0.0f);
        static bool StopActiveSound(uint64_t audioComponentID);
        static bool PauseActiveSound(uint64_t audioComponentID);

        static bool IsPlaying(uint64_t audioComponentID);
    };
}