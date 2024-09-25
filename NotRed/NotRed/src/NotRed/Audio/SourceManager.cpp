#include "nrpch.h"
#include "SourceManager.h"

#include "AudioEngine.h"

namespace NR::Audio
{
    SourceManager::SourceManager(AudioEngine& audioEngine)
        : mAudioEngine(audioEngine) {}

    SourceManager::~SourceManager()
    {
    }

    void SourceManager::Initialize()
    {
    }

    bool SourceManager::InitializeSource(unsigned int sourceID, const SoundConfig& sourceConfig)
    {
        return mAudioEngine.mSoundSources.at(sourceID)->InitializeDataSource(sourceConfig, &mAudioEngine);
    }

    void SourceManager::ReleaseSource(unsigned int sourceID)
    {
        mFreeSourcIDs.push(sourceID);
    }

    bool SourceManager::GetFreeSourceId(int& sourceIDOut)
    {
        if (mFreeSourcIDs.empty())
        {
            return false;
        }

        sourceIDOut = mFreeSourcIDs.front();
        mFreeSourcIDs.pop();

        return true;
    }
}