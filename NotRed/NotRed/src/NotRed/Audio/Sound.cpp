#include "nrpch.h"
#include "Sound.h"

#include "AudioEngine.h"

#include "NotRed/Asset/AssetManager.h"

namespace NR
{
    using namespace Audio;

    const std::string Sound::StringFromState(Sound::ESoundPlayState state)
    {
        std::string str;

        switch (state)
        {
        case ESoundPlayState::Stopped:
            str = "Stopped";
            break;
        case ESoundPlayState::Starting:
            str = "Starting";
            break;
        case ESoundPlayState::Playing:
            str = "Playing";
            break;
        case ESoundPlayState::Pausing:
            str = "Pausing";
            break;
        case ESoundPlayState::Paused:
            str = "Paused";
            break;
        case ESoundPlayState::Stopping:
            str = "Stopping";
            break;
        case ESoundPlayState::FadingOut:
            str = "FadingOut";
            break;
        case ESoundPlayState::FadingIn:
            str = "FadingIn";
            break;
        default:
            break;
        }
        return str;
    }

#if 0 // Enable logging of playback state changes
#define LOG_PLAYBACK(...) NR_CORE_INFO(__VA_ARGS__)
#else
#define LOG_PLAYBACK(...)
#endif

    //==============================================================================

    Sound::~Sound()
    {
        // Technically, we should never have to unitit source here, because they are persistant with pool
        // and uninitialized when the engine is shut down. At least at this moment.
    }

    bool Sound::InitializeDataSource(const Ref<SoundConfig>& config, AudioEngine* audioEngine)
    {
        NR_CORE_ASSERT(!IsPlaying());
        NR_CORE_ASSERT(!bIsReadyToPlay);

        // Reset Finished flag so that we don't accidentally release this voice again while it's starting for the new source
        bFinished = false;

        // TODO: handle passing in different flags for decoding (from data source asset)
        // TODO: and handle decoding somewhere else, in some other way

        if (!config->FileAsset)
            return false;

        auto& assetMetadata = AssetManager::GetMetadata(config->FileAsset->Handle);

        std::string filepath = AssetManager::GetFileSystemPathString(assetMetadata);
        mDebugName = Utils::GetFilename(filepath);

        ma_result result;

        ma_uint32 flags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION;
        result = ma_sound_init_from_file(&audioEngine->mEngine, filepath.c_str(), flags, nullptr, nullptr, &mSound);

        if (result != MA_SUCCESS)
            return false;

        InitializeEffects(config);

        // TODO: handle using parent's (parent group) spatialization vs override (config probably would be passed down here from parrent)
        const bool isSpatializationEnabled = config->bSpatializationEnabled;

        // Setting base Volume and Pitch
        mVolume = (double)config->VolumeMultiplier;
        mPitch = (double)config->PitchMultiplier;
        ma_sound_set_volume(&mSound, config->VolumeMultiplier);
        ma_sound_set_pitch(&mSound, config->PitchMultiplier);

        SetLooping(config->bLooping);

        bIsReadyToPlay = result == MA_SUCCESS;
        return result == MA_SUCCESS;
    }

    void Sound::InitializeEffects(const Ref<SoundConfig>& config)
    {
        ma_node_base* currentHeaderNode = &mSound.engineNode.baseNode;

        // TODO: implement a proper init-uninit routine in SourceManager

        // High-Pass, Low-Pass filters

        mLowPass.Initialize(mSound.engineNode.pEngine, currentHeaderNode);
        currentHeaderNode = mLowPass.GetNode();

        mHighPass.Initialize(mSound.engineNode.pEngine, currentHeaderNode);
        currentHeaderNode = mHighPass.GetNode();

        mLowPass.SetCutoffValue(config->LPFilterValue);
        mHighPass.SetCutoffValue(config->HPFilterValue);

        // Reverb send

        ma_result result = MA_SUCCESS;
        uint32_t numChannels = ma_node_get_output_channels(&mSound, 0);
        ma_splitter_node_config splitterConfig = ma_splitter_node_config_init(numChannels);
        result = ma_splitter_node_init(mSound.engineNode.baseNode.pNodeGraph,
            &splitterConfig,
            &mSound.engineNode.pEngine->pResourceManager->config.allocationCallbacks,
            &mMasterSplitter);

        NR_CORE_ASSERT(result == MA_SUCCESS);

        // Store the node the sound was connected to
        auto* oldOutput = currentHeaderNode->pOutputBuses[0].pInputNode;

        // Attach splitter node to the old output of the sound
        result = ma_node_attach_output_bus(&mMasterSplitter, 0, oldOutput, 0);
        NR_CORE_ASSERT(result == MA_SUCCESS);

        // Attach sound node to splitter node
        result = ma_node_attach_output_bus(currentHeaderNode, 0, &mMasterSplitter, 0);
        NR_CORE_ASSERT(result == MA_SUCCESS);
        //result = ma_node_attach_output_bus(&mMasterSplitter, 0, oldOutput, 0);

        // Set volume of the main pass-through output of the splitter to 1.0
        result = ma_node_set_output_bus_volume(&mMasterSplitter, 0, 1.0f);
        NR_CORE_ASSERT(result == MA_SUCCESS);

        // Mute the "FX send" output of the splitter
        result = ma_node_set_output_bus_volume(&mMasterSplitter, 1, 0.0f);
        NR_CORE_ASSERT(result == MA_SUCCESS);

        /* TODO: Refactore this to
            - InitializeFXSend()
            - GetFXSendNode()
            - ...
          */
    }

    void Sound::ReleaseResources()
    {
        // Duck-tape because uninitializing uninitialized na_nodes causes issues
        if (bIsReadyToPlay)
        {
            // Need to check if hasn't already been uninitialized by the engine (or has been initialized at all)
            if (mSound.engineNode.baseNode.pCachedData != NULL && mSound.pDataSource != NULL)
                ma_sound_uninit(&mSound);

            if (mMasterSplitter.base.pCachedData != NULL && mMasterSplitter.base._pHeap != NULL)
            {
                ma_allocation_callbacks* allocationCallbacks = &mSound.engineNode.pEngine->pResourceManager->config.allocationCallbacks;
                // ma_engine could have been uninitialized by this point, in which case allocation callbacs would've been invalidated
                if (!allocationCallbacks->onFree)
                    allocationCallbacks = nullptr;

                ma_splitter_node_uninit(&mMasterSplitter, allocationCallbacks);
            }

            mLowPass.Uninitialize();
            mHighPass.Uninitialize();

            bIsReadyToPlay = false;
        }
    }

    bool Sound::Play()
    {
        if (!IsReadyToPlay())
            return false;

        ma_result result = MA_ERROR;
        LOG_PLAYBACK("Old state: " + StringFromState(mPlayState));

        switch (mPlayState)
        {
        case ESoundPlayState::Stopped:
            bFinished = false;
            result = ma_sound_start(&mSound);
            mPlayState = ESoundPlayState::Starting;
            break;
        case ESoundPlayState::Starting:
            NR_CORE_ASSERT(false);
            break;
        case ESoundPlayState::Playing:
            StopNow(false, true);
            bFinished = false;
            result = ma_sound_start(&mSound);
            mPlayState = ESoundPlayState::Starting;
            break;
        case ESoundPlayState::Pausing:
            StopNow(false, false);
            result = ma_sound_start(&mSound);
            mPlayState = ESoundPlayState::Starting;
            break;
        case ESoundPlayState::Paused:
            // Prepare fade-in for un-pause
            ma_sound_set_fade_in_milliseconds(&mSound, 0.0f, mStoredFaderValue, STOPPING_FADE_MS / 2);
            result = ma_sound_start(&mSound);
            mPlayState = ESoundPlayState::Starting;
            break;
        case ESoundPlayState::Stopping:
            StopNow(true, true);
            result = ma_sound_start(&mSound);
            mPlayState = ESoundPlayState::Starting;
            break;
        case ESoundPlayState::FadingOut:
            break;
        case ESoundPlayState::FadingIn:
            break;
        default:
            break;
        }
        LOG_PLAYBACK("New state: " + StringFromState(mPlayState));
        NR_CORE_ASSERT(result == MA_SUCCESS);
        return result == MA_SUCCESS;

        // TODO: consider marking this sound for stop-fade and switching to new one to prevent click on restart
        //       or some other, lower level solution to fade-out stopping while starting to read from the beginning

    }

    bool Sound::Stop()
    {
        bool result = true;
        switch (mPlayState)
        {
        case ESoundPlayState::Stopped:
            bFinished = true;
            result = false;
            break;
        case ESoundPlayState::Starting:
            StopNow(false, true); // consider stop-fading
            mPlayState = ESoundPlayState::Stopping;
            break;
        case ESoundPlayState::Playing:
            result = StopFade(STOPPING_FADE_MS);
            mPlayState = ESoundPlayState::Stopping;
            break;
        case ESoundPlayState::Pausing:
            StopNow(true, true);
            mPlayState = ESoundPlayState::Stopping;
            break;
        case ESoundPlayState::Paused:
            StopNow(true, true);
            mPlayState = ESoundPlayState::Stopping;
            break;
        case ESoundPlayState::Stopping:
            StopNow(true, true);
            break;
        case ESoundPlayState::FadingOut:
            break;
        case ESoundPlayState::FadingIn:
            break;
        default:
            break;
        }
        mLastFadeOutDuration = (float)STOPPING_FADE_MS / 1000.0f;
        LOG_PLAYBACK(StringFromState(mPlayState));
        return result;
    }

    bool Sound::Pause()
    {
        bool result = true;

        switch (mPlayState)
        {
        case ESoundPlayState::Playing:
            result = StopFade(STOPPING_FADE_MS);
            mPlayState = ESoundPlayState::Pausing;
            break;
        case ESoundPlayState::FadingOut:
            break;
        case ESoundPlayState::FadingIn:
            break;
        default:
            // If was called to Pause while not playing
            mPlayState = ESoundPlayState::Paused;
            result = true;
            break;
        }
        LOG_PLAYBACK(StringFromState(mPlayState));

        return result;
    }

    bool Sound::StopFade(uint64_t numSamples)
    {
        constexpr double stopFadeTime = (double)STOPPING_FADE_MS * 1.1 / 1000.0;
        mStopFadeTime = stopFadeTime;

        // Negative volumeBeg starts the fade from the current volume
        ma_sound_set_fade_in_pcm_frames(&mSound, -1.0f, 0.0f, numSamples);

        return true;
    }

    bool Sound::StopFade(int milliseconds)
    {
        NR_CORE_ASSERT(milliseconds > 0);

        const uint64_t fadeInFrames = (milliseconds * mSound.engineNode.fader.config.sampleRate) / 1000;

        return StopFade(fadeInFrames);
    }

    bool Sound::FadeIn(const float duration, const float targetVolume)
    {
        /*if (IsPlaying() || bStopping)
            return false;

        ma_result result;
        result = ma_sound_set_fade_in_milliseconds(&mSound, 0.0f, targetVolume, uint64_t(duration * 1000));
        if (result != MA_SUCCESS)
            return false;

        bPaused = false;
        mStoredFaderValue = targetVolume;*/

        return ma_sound_start(&mSound) == MA_SUCCESS;
    }

    bool Sound::FadeOut(const float duration, const float targetVolume)
    {
        //if (!IsPlaying())
        //    return false;

        //// If fading out not completely, store the end value to reference later as "current" value
        //if(targetVolume != 0.0f)
        //    mStoredFaderValue = targetVolume;

        //mLastFadeOutDuration = duration;
        //bFadingOut = true;
        //StopFade(int(duration * 1000));

        return true;
    }

    bool Sound::IsPlaying() const
    {
        return mPlayState != ESoundPlayState::Stopped && mPlayState != ESoundPlayState::Paused;
    }

    bool Sound::IsFinished() const
    {
        return bFinished/* && !bPaused*/;
    }

    bool Sound::IsStopping() const
    {
        return mPlayState == ESoundPlayState::Stopping;
    }

    void Sound::SetVolume(float newVolume)
    {
        ma_sound_set_volume(&mSound, mVolume * (double)newVolume);
    }

    void Sound::SetPitch(float newPitch)
    {
        ma_sound_set_pitch(&mSound, mPitch * (double)newPitch);
    }

    void Sound::SetLooping(bool looping)
    {
        bLooping = looping;
        ma_sound_set_looping(&mSound, bLooping);
    }

    float Sound::GetVolume()
    {
        return ma_node_get_output_bus_volume(&mSound, 0);
    }

    float Sound::GetPitch()
    {
        return mSound.engineNode.pitch;
    }

    void Sound::SetLocation(const glm::vec3& location, const glm::vec3& orientation)
    {
#ifdef SPATIALIZER_TEST

        ma_sound_set_position(&mSound, location.x, location.y, location.z);
#else     
        //if(mSpatializer.IsInitialized())
           // mSpatializer.SetPositionRotation(location, orientation);
#endif
        //mAirAbsorptionFilter.UpdateDistance(glm::distance(location, glm::vec3(0.0f, 0.0f, 0.0f)));
    }

    void Sound::SetVelocity(const glm::vec3& velocity /*= { 0.0f, 0.0f, 0.0f }*/)
    {
    }

    void Sound::Update(float dt)
    {
        auto notifyIfFinished = [&]
            {
                if (ma_sound_at_end(&mSound) == MA_TRUE && onPlaybackComplete)
                    onPlaybackComplete();
            };

        auto isNodePlaying = [&]
            {
                return ma_sound_is_playing(&mSound) == MA_TRUE;
            };

        auto isFadeFinished = [&]
            {
                return mStopFadeTime <= 0.0;
            };

        mStopFadeTime = std::max(0.0, mStopFadeTime - (double)dt);

        switch (mPlayState)
        {
        case ESoundPlayState::Stopped:
            break;
        case ESoundPlayState::Starting:
            if (isNodePlaying())
            {
                mPlayState = ESoundPlayState::Playing;
                LOG_PLAYBACK("Upd: " + StringFromState(mPlayState));
            }
            break;
        case ESoundPlayState::Playing:
            if (ma_sound_is_playing(&mSound) == MA_FALSE)
            {
                mPlayState = ESoundPlayState::Stopped;
                bFinished = true;
                notifyIfFinished();
                LOG_PLAYBACK("Upd: " + StringFromState(mPlayState));
            }
            break;
        case ESoundPlayState::Pausing:
            if (isFadeFinished())
            {
                StopNow(false, false);
                mPlayState = ESoundPlayState::Paused;
                LOG_PLAYBACK("Upd: " + StringFromState(mPlayState));
            }
            break;
        case ESoundPlayState::Paused:
            break;
        case ESoundPlayState::Stopping:
            if (isFadeFinished())
            {
                StopNow(true, true);
                mPlayState = ESoundPlayState::Stopped;
                LOG_PLAYBACK("Upd: " + StringFromState(mPlayState));
            }
            break;
        case ESoundPlayState::FadingOut:
            break;
        case ESoundPlayState::FadingIn:
            break;
        default:
            break;
        }
    }

    int Sound::StopNow(bool notifyPlaybackComplete /*= true*/, bool resetPlaybackPosition /*= true*/)
    {
        // Stop reading the data source
        ma_sound_stop(&mSound);

        if (resetPlaybackPosition)
        {
            // Reset data source read position to the beginning of the data
            ma_sound_seek_to_pcm_frame(&mSound, 0);

            // Mark this voice to be released.
            bFinished = true;

            NR_CORE_ASSERT(mPlayState != ESoundPlayState::Starting);
            mPlayState = ESoundPlayState::Stopped;
        }
        mSound.engineNode.fader.volumeEnd = 1.0f;

        // Need to notify AudioEngine of completion,
        // if this is one shot, AudioComponent needs to be destroyed.
        if (notifyPlaybackComplete && onPlaybackComplete)
            onPlaybackComplete();

        return mSoundSourceID;
    }

    float Sound::GetCurrentFadeVolume()
    {
        // TODO: return volume accounted for distance attenuation, or better read output volume envelope.
        float currentVolume = ma_sound_get_current_fade_volume(&mSound);

        return currentVolume;
    }

    float Sound::GetPriority()
    {
        return GetCurrentFadeVolume() * ((float)mPriority / 255.0f);
    }

    float Sound::GetPlaybackPercentage()
    {
        ma_uint64 currentFrame;
        ma_uint64 totalFrames;
        ma_sound_get_cursor_in_pcm_frames(&mSound, &currentFrame);

        // TODO: concider storing this value when initializing sound;
        ma_sound_get_length_in_pcm_frames(&mSound, &totalFrames);

        return (float)currentFrame / (float)totalFrames;
    }

} // namespace NotRed