#include "nrpch.h"
#include "SourceManager.h"

#include "AudioEngine.h"
#include "NotRed/Audio/DSP/Reverb/Reverb.h"

namespace NR::Audio
{
    void SourceManager::AllocationCallback(uint64_t size)
    {
        auto& stats = AudioEngine::Get().sStats;
        std::scoped_lock lock{ stats.mutex };
        stats.MemResManager += size;
    }

    void SourceManager::DeallocationCallback(uint64_t size)
    {
        auto& stats = AudioEngine::Get().sStats;
        std::scoped_lock lock{ stats.mutex };
        stats.MemResManager -= size;
    }

    SourceManager::SourceManager(AudioEngine& audioEngine)
        : mAudioEngine(audioEngine) {}

    SourceManager::~SourceManager()
    {
    }

    void SourceManager::Initialize()
    {
    }

    bool SourceManager::InitializeSource(uint32_t sourceID, const SoundConfig& sourceConfig)
    {
        if (mAudioEngine.mSoundSources.at(sourceID)->InitializeDataSource(sourceConfig, &mAudioEngine))
        {
            auto* reverbNode = mAudioEngine.GetMasterReverb()->GetNode();
            auto& splitterNode = mAudioEngine.mSoundSources.at(sourceID)->mMasterSplitter;

            // Attach "FX send" output of the splitter to the Master Reverb node
            ma_node_attach_output_bus(&splitterNode, 1, reverbNode, 0);

            // Set send level to the Master Reverb
            ma_node_set_output_bus_volume(&splitterNode, 1, sourceConfig.MasterReverbSend);

            return true;
        }

        return false;
    }

    void SourceManager::ReleaseSource(uint32_t sourceID)
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

    void SourceManager::SetMasterReverbSendForSource(uint32_t sourceID, float sendLevel)
    {
        auto& engine = AudioEngine::Get();
        auto* reverbNode = engine.GetMasterReverb()->GetNode();
        auto& splitterNode = engine.mSoundSources.at(sourceID)->mMasterSplitter;
        
        // Set send level to the Master Reverb
        ma_node_set_output_bus_volume(&splitterNode, 1, sendLevel);
    }

    void SourceManager::SetMasterReverbSendForSource(Sound* source, float sendLevel)
    {
        SetMasterReverbSendForSource(source->mSoundSourceID, sendLevel);
    }
}