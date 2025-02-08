#pragma once

#include "SoundObject.h"
#include "miniaudioInc.h"

#include <glm/glm.hpp>

#include "NotRed/Asset/Asset.h"
#include "NotRed/Asset/AssetTypes.h"

#include "DSP/Spatializer/Spatializer.h"
#include "DSP/Filters/LowPassFilter.h"
#include "DSP/Filters/HighPassFilter.h"

namespace NR
{
    class AudioEngine;

    enum class AttenuationModel
    {
        None,          // No distance attenuation and no spatialization.
        Inverse,       // Equivalent to OpenAL's AL_INVERSE_DISTANCE_CLAMPED.
        Linear,        // Linear attenuation. Equivalent to OpenAL's AL_LINEAR_DISTANCE_CLAMPED.
        Exponential    // Exponential attenuation. Equivalent to OpenAL's AL_EXPONENT_DISTANCE_CLAMPED.

        // TODO: CusomCurve
    };

    /* ==============================================
        Configuration for 3D spatialization behavior
        ---------------------------------------------
     */
    struct SpatializationConfig : public Asset
    {
        AttenuationModel AttenuationMod{ AttenuationModel::Inverse };   // Distance attenuation function
        float MinGain{ 0.0f };                                            // Minumum volume muliplier
        float MaxGain{ 1.0f };                                            // Maximum volume multiplier
        float MinDistance{ 1.0f };                                        // Distance where to start attenuation
        float MaxDistance{ 1000.0f };                                     // Distance where to end attenuation
        float ConeInnerAngleInRadians{ 6.283185f };                     // Defines the angle where no directional attenuation occurs 
        float ConeOuterAngleInRadians{ 6.283185f };                     // Defines the angle where directional attenuation is at max value (lowest multiplier)
        float ConeOuterGain{ 0.0f };                                      // Attenuation multiplier when direction of the emmiter falls outside of the ConeOuterAngle
        float DopplerFactor{ 1.0f };                                      // The amount of doppler effect to apply. Set to 0 to disables doppler effect. 
        float Rolloff{ 0.6f };                                            // Affects steepness of the attenuation curve. At 1.0 Inverse model is the same as Exponential

        bool bAirAbsorptionEnabled{ true };                            // Enable Air Absorption filter (TODO)

        bool bSpreadFromSourceSize{ true };                             // If this option is enabled, spread of a sound source automatically calculated from the source size.
        float SourceSize{ 1.0f };                                       // Diameter of the sound source in game world.
        float Spread{ 1.0f };
        float Focus{ 1.0f };

        static AssetType GetStaticType() { return AssetType::SpatializationConfig; }
        virtual AssetType GetAssetType() const { return GetStaticType(); }
    };


    /* ==============================================
        Sound configuration to initialize sound source

        Can be passed to an AudioComponent              (TODO: or to directly initialize Sound to play)
        This represents a basic sound to play
        ----------------------------------------------
    */
    struct SoundConfig : public Asset
    {
        AssetHandle FileAsset;     // Audio data source

        bool bLooping = false;
        float VolumeMultiplier = 1.0f;
        float PitchMultiplier = 1.0f;

        bool bSpatializationEnabled = false;
        Ref<SpatializationConfig> Spatialization{ new SpatializationConfig() };        // Configuration for 3D spatialization behavior

        float MasterReverbSend = 0.0f;
        float LPFilterValue = 1.0f;
        float HPFilterValue = 0.0f;

        static AssetType GetStaticType() { return AssetType::SoundConfig; }
        virtual AssetType GetAssetType() const override { return GetStaticType(); }
    };



    /* =====================================
        Basic Sound, represents playing voice

        -------------------------------------
    */
    class Sound : public Audio::SoundObject
    {
    public:
        Sound() = default;
        ~Sound();

        //--- Sound Source Interface
        virtual bool Play() override;
        virtual bool Stop() override;
        virtual bool Pause() override;
        virtual bool IsPlaying() const override;
        // ~ End of Sound Source Interface

        virtual void SetVolume(float newVolume) override;
        virtual void SetPitch(float newPitch) override;
        void SetLooping(bool looping);

        virtual float GetVolume() override;
        virtual float GetPitch() override;

        virtual bool FadeIn(const float duration, const float targetVolume) override;
        virtual bool FadeOut(const float duration, const float targetVolume) override;

        /* Initialize data source from sound configuration.
            @param soundConfig - configuration to initialized data source with
            @param audioEngine - reference to AudioEngine

            @returns true - if successfully initialized data source
         */
        bool InitializeDataSource(const Ref<SoundConfig>& soundConfig, AudioEngine* audioEngine);

        void ReleaseResources();

        void SetLocation(const glm::vec3& location, const glm::vec3& orientation);
        void SetVelocity(const glm::vec3& velocity = { 0.0f, 0.0f, 0.0f });

        /* @return true - if has a valid data source */
        bool IsReadyToPlay() const { return bIsReadyToPlay; }

        void Update(float dt) override;

        /* @returns true - if playback complete. E.g. reached the end of data, or has been hard-stopped. */
        bool IsFinished() const;

        /* @returns true - if currently "stop-fading". */
        bool IsStopping() const;

        /* Get current level of fader performing fade operations.
           Such operations performed when FadeIn(), or FadeOut() are called,
           as well as "stop-fade" and "resume from pause" fade.

           @returns current fader level
        */
        float GetCurrentFadeVolume();

        /* Get current priority level based on current fade volume
           and priority value set for this sound source.

           @returns normalized priority value
        */
        float GetPriority();

        /* @returns current playback percentage (read position) whithin data source */
        float GetPlaybackPercentage();

    private:
        /* Stop playback with short fade-out to prevent click.
           @param numSamples - length of the fade-out in PCM frames

           @returns true - if successfully initialized fade
        */
        bool StopFade(uint64_t numSamples);

        /* Stop playback with short fade-out to prevent click.
           @param milliseconds - length of the fade-out in milliseconds

           @returns true - if successfully initialized fade
        */
        bool StopFade(int milliseconds);

        /* "Hard-stop" playback without fade. This is called to immediatelly stop playback, has ended,
           as well as to reset the play state when "stop-fade" has ended.
           @param notifyPlaybackComplete - used when the sound has finished its playback
           @param resetPlaybackPosition - set to 'false' when pausing

           @returns ID of the sound source in pool
        */
        int StopNow(bool notifyPlaybackComplete = true, bool resetPlaybackPosition = true);

        void InitializeEffects(const Ref<SoundConfig>& config);


    private:
        friend class AudioEngine;
        friend class SourceManager;

        std::function<void()> onPlaybackComplete;
        Ref<SoundConfig> mSoundConfig;
        std::string mDebugName;

        enum class ESoundPlayState
        {
            Stopped,
            Starting,
            Playing,
            Pausing,
            Paused,
            Stopping,
            FadingOut,
            FadingIn
        };
        static const std::string StringFromState(Sound::ESoundPlayState state);

        ESoundPlayState mPlayState{ ESoundPlayState::Stopped };


        /* ID of this sound source in pool of pre-initialized sources. */
        int mSoundSourceID = -1;

        /* Data source. For now all we use is Miniaudio,
            in the future SoundObject will access data source via SoundSource class.
         */
        ma_sound mSound;
        ma_splitter_node mMasterSplitter;

        bool bIsReadyToPlay = false;

        // TODO
        uint8_t mPriority = 64;

        bool bLooping = false;
        bool bFinished = false;

        /* Stored Fader "resting" value. Used to restore Fader before restarting playback if a fade has occured. */
        float mStoredFaderValue = 1.0f;
        float mLastFadeOutDuration = 0.0f;

        double mVolume = 1.0f;
        double mPitch = 1.0f;

        /* Stop-fade counter. Used to stop the sound after "stopping-fade" has finished. */
        double mStopFadeTime = 0.0;

        /* ID of the AudioComponent this voice was initialized from and is attached to */
        ////uint64_t mAudioComponentID = 0;
        uint64_t mAudioObjectID = 0;
        uint32_t mEventID = 0;

        /* ID of the scene this voice belongs to. */
        uint64_t mSceneID = 0;

        Audio::DSP::LowPassFilter mLowPass;
        Audio::DSP::HighPassFilter mHighPass;

        //? TESTING
        //DSP::AirAbsorptionFilter mAirAbsorptionFilter;
    };

} // namespace NotRed