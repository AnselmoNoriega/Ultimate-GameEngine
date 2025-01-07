#include "nrpch.h"
#include "SourceManager.h"

#include "AudioEngine.h"
#include "DSP/Reverb/Reverb.h"
#include "DSP/Spatializer/Spatializer.h"

namespace NR
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

    //==================================================================================

    SourceManager::SourceManager(AudioEngine& audioEngine)
        : mAudioEngine(audioEngine)
    {
        mSpatializer = CreateScope<Audio::DSP::Spatializer>();
    }

    SourceManager::~SourceManager()
    {
        // TODO: release all of the sources
    }

    void SourceManager::Initialize()
    {
        mSpatializer->Initialize(&mAudioEngine.mEngine);
    }

    void SourceManager::UninitializeEffects()
    {
        for (auto* source : mAudioEngine.mSoundSources)
        {
            ReleaseSource(source->mSoundSourceID);
        }

    }

    bool SourceManager::InitializeSource(uint32_t sourceID, const Ref<SoundConfig>& sourceConfig)
    {
        if (mAudioEngine.mSoundSources.at(sourceID)->InitializeDataSource(sourceConfig, &mAudioEngine))
        {
            auto* reverbNode = mAudioEngine.GetMasterReverb()->GetNode();
            auto* soundSource = mAudioEngine.mSoundSources.at(sourceID);

            // Initialize spatialization source
            if (sourceConfig->bSpatializationEnabled)
                mSpatializer->InitSource(sourceID, &soundSource->mSound.engineNode, sourceConfig->Spatialization);


            //--- Set up Master Reverb send for the new source ---
            //----------------------------------------------------
            // TODO: Temp. In the future move this to Effects interface
            auto& splitterNode = soundSource->mMasterSplitter;

            // Attach "FX send" output of the splitter to the Master Reverb node
            ma_node_attach_output_bus(&splitterNode, 1, reverbNode, 0);
            // Set send level to the Master Reverb
            ma_node_set_output_bus_volume(&splitterNode, 1, sourceConfig->MasterReverbSend);

            return true;
        }
        else
        {
            NR_CORE_ASSERT(false, "Failed to initialize audio data soruce.");
        }

        return false;
    }

    void SourceManager::ReleaseSource(uint32_t sourceID)
    {
        mSpatializer->ReleaseSource(sourceID);
        mAudioEngine.mSoundSources.at(sourceID)->ReleaseResources();

        mFreeSourcIDs.push(sourceID);
    }

    bool SourceManager::GetFreeSourceId(int& sourceIdOut)
    {
        if (mFreeSourcIDs.empty())
            return false;

        sourceIdOut = mFreeSourcIDs.front();
        mFreeSourcIDs.pop();

        return true;
    }

    void SourceManager::SetMasterReverbSendForSource(uint32_t sourceID, float sendLevel)
    {
        // TODO: Temp. In the future move this to Effects interface

        auto& engine = AudioEngine::Get();
        //auto* reverbNode = engine.GetMasterReverb()->GetNode();
        auto& splitterNode = engine.mSoundSources.at(sourceID)->mMasterSplitter;

        // Set send level to the Master Reverb
        ma_node_set_output_bus_volume(&splitterNode, 1, sendLevel);
    }

    void SourceManager::SetMasterReverbSendForSource(Sound* source, float sendLevel)
    {
        SetMasterReverbSendForSource(source->mSoundSourceID, sendLevel);
    }

    float SourceManager::GetMasterReverbSendForSource(Sound* source)
    {
        auto& engine = AudioEngine::Get();
        auto& splitterNode = engine.mSoundSources.at(source->mSoundSourceID)->mMasterSplitter;

        return ma_node_get_output_bus_volume(&splitterNode, 1);
    }

} // namespace