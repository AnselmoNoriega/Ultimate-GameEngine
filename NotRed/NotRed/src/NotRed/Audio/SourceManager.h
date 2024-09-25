#pragma once

#include "Sound.h"
#include <queue>

namespace NR::Audio
{
    class MiniAudioEngine;

    class SourceManager
    {
    public:
        SourceManager(MiniAudioEngine& audioEngine);
        ~SourceManager();

        void Initialize();
        bool InitializeSource(unsigned int sourceID, const SoundConfig& sourceConfig);

        void ReleaseSource(unsigned int sourceID);

        bool GetFreeSourceId(int& sourceIDOut);

    private:
        MiniAudioEngine& mAudioEngine;
        std::queue<int> mFreeSourcIDs;

    private:
        friend class MiniAudioEngine;
    };
}