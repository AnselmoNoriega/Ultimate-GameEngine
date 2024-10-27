#pragma once
#include "Sound.h"

namespace NR::Audio
{
    class AudioPlayback
    {
    public:
        static bool PlaySound2D(const Ref<SoundConfig>& soundConfig, float volume = 1.0f, float pitch = 1.0f);
        static bool PlaySound2D(Ref<AudioFile> soundAsset, float volume = 1.0f, float pitch = 1.0f);

        static bool PlaySoundAtLocation(const Ref<SoundConfig>& soundConfig, glm::vec3 location, float volume = 1.0f, float pitch = 1.0f);
        static bool PlaySoundAtLocation(Ref<AudioFile> soundAsset, glm::vec3 location, float volume = 1.0f, float pitch = 1.0f);

        static bool Play(uint64_t audioComponentID, float startTime = 0.0f);
        static bool StopActiveSound(uint64_t audioComponentID);
        static bool PauseActiveSound(uint64_t audioComponentID);

        static bool IsPlaying(uint64_t audioComponentID);

        static void SetMasterReverbSend(uint64_t audioComponetnID, float sendLevel);
        static float GetMasterReverbSend(uint64_t audioComponetnID);
    };
}