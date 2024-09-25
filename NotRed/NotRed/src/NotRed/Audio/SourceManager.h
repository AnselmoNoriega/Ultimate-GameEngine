#pragma once

#include "Sound.h"
#include <queue>

namespace NR::Audio
{
    class AudioEngine;

    class SourceManager
    {
    public:
        SourceManager(AudioEngine& audioEngine);
        ~SourceManager();

        void Initialize();
        bool InitializeSource(unsigned int sourceID, const SoundConfig& sourceConfig);

        void ReleaseSource(unsigned int sourceID);

        bool GetFreeSourceId(int& sourceIDOut);

    private:
        AudioEngine& mAudioEngine;
        std::queue<int> mFreeSourcIDs;

    private:
        friend class AudioEngine;
    };
}