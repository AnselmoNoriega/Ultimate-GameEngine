#pragma once

#include <optional>
#include <queue>
#include <shared_mutex>
#include <thread>

#include <glm/glm.hpp>

#include "MiniAudio/include/miniaudioInc.h"

#include "NotRed/Core/Timer.h"
#include "NotRed/Scene/Scene.h"
#include "NotRed/Scene/Entity.h"

#include "Audio.h"
#include "SourceManager.h"
#include "Sound.h"
#include "AudioObject.h"
#include "EntityIDMaps.h"
#include "AudioEvents/AudioCommands.h"
#include "AudioEvents/CommandID.h"
#include "AudioComponent.h"
#include "AudioPlayback.h"

#include "NotRed/Scene/Components.h"

namespace YAML
{
    class Emitter;
    class Node;
}

namespace NR
{
    class SourceManager;
    class Sound;
    struct AudioComponent;
    class AudioObject;

    namespace Audio::DSP
    {
        struct Reverb;
    }

    namespace Audio
    {
        struct Stats
        {
            Stats() = default;
            Stats(const Stats& other)
            {
                std::shared_lock lock{ other.mutex };
                AudioObjects = other.AudioObjects;
                ActiveEvents = other.ActiveEvents;
                NumActiveSounds = other.NumActiveSounds;
                TotalSources = other.TotalSources;
                MemEngine = other.MemEngine;
                MemResManager = other.MemResManager;
                FrameTime = other.FrameTime;
                NumAudioComps = other.NumAudioComps;
            }

            uint32_t AudioObjects = 0;
            uint32_t ActiveEvents = 0;
            uint32_t NumActiveSounds = 0;
            uint32_t TotalSources = 0;
            uint64_t MemEngine = 0;
            uint64_t MemResManager = 0;
            float FrameTime = 0.0f;
            uint64_t NumAudioComps = 0;

            mutable std::shared_mutex mutex;
        };


        /* ----------------------------------------------------------------
            For now just a flag to pass into the memory allocation callback
            to sort allocation stats accordingly to the source of operation
            ---------------------------------------------------------------
        */
        struct AllocationCallbackData // TODO: hide this into Source Manager?
        {
            bool isResourceManager = false;
            Stats& Stats;
        };

    } // namespace Audio

    /*  =================================================================
        Object representing current state of the Audio Listener

        Updated from Game Thead, checked and updated to from Audio Thread
        -----------------------------------------------------------------
    */
    struct AudioListener
    {
        bool PositionNeedsUpdate(const Audio::Transform& newTransform) const
        {
            std::shared_lock lock{ mMutex };
            //return newPosition != mLastPosition || newDirection != mLastDirection;
            return mTransform != newTransform;
        }

        void SetNewPositionDirection(const Audio::Transform& newTransform)
        {
            std::unique_lock lock{ mMutex };
            //mLastPosition = newPosition;
            //mLastDirection = newDirection;
            mTransform = newTransform;
            mChanged = true;
        }

        void SetNewVelocity(const glm::vec3& newVelocity)
        {
            std::unique_lock lock{ mMutex };
            mChanged = newVelocity != mLastVelocity;
            mLastVelocity = newVelocity;
        }

        void SetNewConeAngles(float innerAngle, float outerAngle, float outerGain)
        {
            mInnerAngle.store(innerAngle);
            mOuterAngle.store(outerAngle);
            mOuterGain.store(outerGain);
            mChanged = true;
        }

        [[nodiscard]] std::pair<float, float> GetConeInnerOuterAngles() const
        {
            return { mInnerAngle.load(), mOuterAngle.load() };
        }

        [[nodiscard]] float GetConeOuterGain() const
        {
            return mOuterGain.load();
        }

        void GetVelocity(glm::vec3& velocity) const
        {
            std::shared_lock lock{ mMutex };
            velocity = mLastVelocity;
        }

        Audio::Transform GetPositionDirection() const
        {
            std::shared_lock lock{ mMutex };
            //position = mLastPosition;
            //direction = mLastDirection;
            return mTransform;
        }

        bool HasChanged(bool resetFlag) const
        {
            bool changed = mChanged.load();
            if (resetFlag) mChanged.store(false);
            return changed;
        }

    private:
        mutable std::shared_mutex mMutex;
        glm::vec3 mLastVelocity{ 0.0f, 0.0f, 0.0f };
        Audio::Transform mTransform;
        mutable std::atomic<bool> mChanged = false;

        std::atomic<float> mInnerAngle{ 6.283185f }, mOuterAngle{ 6.283185f }, mOuterGain{ 0.0f };
    };

    /* ========================================
       Generic Audio operations

       Hardware Initialization, main Update hub
       ----------------------------------------
    */

    class AudioEngine
    {
    public:
        AudioEngine();
        ~AudioEngine();

        /* Initialize Instance */
        static void Init();

        /* Shutdown AudioEngine and tear down hardware initialization */
        static void Shutdown();

        static inline AudioEngine& Get() { return *sInstance; }

        static void SetSceneContext(const Ref<Scene>& scene);
        static Ref<Scene>& GetCurrentSceneContext();
        static void OnRuntimePlaying(UUID sceneID);
        static void OnSceneDestruct(UUID sceneID);

        void SerializeSceneAudio(YAML::Emitter& out, const Ref<Scene>& scene);
        void DeserializeSceneAudio(YAML::Node& data);

        static Audio::Stats GetStats();

        /* Initializes AudioEngine and hardware. This is called from AudioEngine instance construct.
            Executed on Audio Thread.

           @returns true - if initialization succeeded, or if AudioEngine already initialized
        */
        bool Initialize();

        /* Tear down hardware initialization. This is called from Shutdown(). Or AudioEngine deconstruct

           @returns true - if initialization succeeded, or if AudioEngine already initialized
        */
        bool Uninitialize();


        //==================================================================================

        void StopAll(bool stopNow = false);
        void PauseAll();
        void ResumeAll();

        /* Stop all Active Sounds
           @param stopNow - stop immediately if set 'true', otherwise apply "stop-fade"
        */
        static void StopAllSounds(bool stopNow = false) { sInstance->StopAll(stopNow); }

        /* Pause Audio Engine. E.g. when game minimized. */
        static void Pause() { sInstance->PauseAll(); }
        /* Resume paused Audio Engine. E.g. when game window restored from minimized state. */
        static void Resume() { sInstance->ResumeAll(); }

        // Playback stated used to pause, or resume all of the audio globaly when game minimized.
        enum class EPlaybackState
        {
            Playing,
            Paused
        } mPlaybackState{ EPlaybackState::Playing }; // Could add in "Initialized" state as well in the future.

        SourceManager& GetSourceManager() { return mSourceManager; }
        const SourceManager& GetSourceManager() const { return mSourceManager; }

        Audio::DSP::Reverb* GetMasterReverb() { return mMasterReverb.get(); }


        //==================================================================================

        /* Main Audio Thread tick update function */
        void Update(float dt);

        /* Submit data to update Sound Sources from Game Thread.
           @param updateData - updated data submitted on scene update
        */
        void SubmitSourceUpdateData(std::vector<SoundSourceUpdateData> updateData);

        /* Update Audio Listener position from game Entity owning active AudioListenerComponent.
            Called from Game Thread.

           @param newTranslation - new position to check Audio Listener against and to update from if changed
           @param newDirection - new facing direction to check Audio Listener against and to update from if changed
        */
        void UpdateListenerPosition(const Audio::Transform& newTransform);

        /* Update Audio Listener velocity from game Entity owning active AudioListenerComponent.
            Called from Game Thread.

           @param newVelocity - new velocity to update Audio Listener from. This doesn't check if changed.
        */
        void UpdateListenerVelocity(const glm::vec3& newVelocity);

        void UpdateListenerConeAttenuation(float innerAngle, float outerAngle, float outerGain);

        // TODO
        void RegisterNewListener(AudioListenerComponent& listenerComponent);

        //==================================================================================

        UUID InitializeAudioObject(UUID objectID, const std::string& debugName, const Audio::Transform& objectPosition = Audio::Transform());
        void ReleaseAudioObject(UUID objectID);
        std::optional<AudioObject*> GetAudioObject(uint64_t objectID);
        std::string GetAudioObjectInfo(uint64_t objectID);
        UUID GetAudioObjectOfComponent(uint64_t audioComponentID);

        //? TEMP. This is not ideal, but thread safe
        bool SetTransform(uint64_t objectID, const Audio::Transform& objectPosition);
        std::optional<Audio::Transform> GetTransform(uint64_t objectID);

        // TODO
        void AttachObject(uint64_t audioComponentID, uint64_t audioObjectID, const glm::vec3& PositionOffset);

        Audio::EventID PostTrigger(Audio::CommandID triggerCommandID, UUID objectID);

        // TODO
        //void SetSwitch(Audio::SwitchCommand switchCommand, Audio::CommandID valueID);
        //void SetState(Audio::StateCommand switchCommand, Audio::CommandID valueID);
        //void SetParameter(Audio::ParameterCommand parameter, double value);

        Sound* InitiateNewVoice(UUID objectID, Audio::EventID playbackID, const Ref<SoundConfig>& sourceConfig);

        //==================================================================================

        // TODO: add blocking vs async versions
        /* Execute arbitrary function on Audio Thread. Used to synchronize updates between Game Thread and Audio Thread */
        static void ExecuteOnAudioThread(Audio::AudioThreadCallbackFunction func, /*void* parameter, */const char* jobID = "NONE");

        /* Add AudioComponent to AudioComponentRegistry for easy access from AduioEngine. */
        void RegisterAudioComponent(Entity entity);

        /* Remove AudioComponent from AudioComponentRegistry. */
        void UnregisterAudioComponent(UUID sceneID, UUID entityID);

    private:
        AudioComponent* GetAudioComponentFromID(UUID sceneID, uint64_t audioComponentID);

        /* Pre-initialize pool of Sound Sources */
        void CreateSources();

        /* Release sources that finished their playback back into pool */
        void ReleaseFinishedSources();

        /* Update back-end audio listener from AudioListener state object */
        void UpdateListener();

        /* Internal function call to update sound sources to new values recieved from Game Thread */
        void UpdateSources();

        /* This is called when there is no free source available in pool for new playback start request. */
        Sound* FreeLowestPrioritySource();


        //==================================================================================
        //=== Playback interface. In most cases these functions are called from Game Thread.

        /*  Internal version of "Play" command. Posts Audio Event assigned to AudioComponent.

            @param audioComponentID - AudioComponent to associate new sound with
            @returns false - if failed to find audioComponentID in AudioComponentRegistry
        */
        bool SubmitSoundToPlay(uint64_t audioComponentID);

        /*  Internal version of "Stop" command. Attempts to stop Active Sources
            initiated by playing events on AudioComponent.

            @returns false - if failed to post event, or if audioComponentID is invalid
        */
        bool StopActiveSoundSource(uint64_t audioComponentID);

        /*  Internal version of "Pause" command. Attempts to pause Active Sources
            initiated by playing events on AudioComponent.

            @returns false - if failed to find audioComponentID in AudioComponentRegistry
        */
        bool PauseActiveSoundSource(uint64_t audioComponentID);

        /*  Internal version of "Resume" command. Attempts to resume Active Sources
             initiated by playing events on AudioComponent.

             @returns false - if failed to find audioComponentID in AudioComponentRegistry
         */
        bool ResumeActiveSoundSource(uint64_t audioComponentID);

        bool StopEventID(Audio::EventID playingEvent);
        bool PauseEventID(Audio::EventID playingEvent);
        bool ResumeEventID(Audio::EventID playingEvent);

        /*  Check if AudioComponent has Active Sources associated to active Events

            @returns false - if there is no ActiveSound associated to the audioComponentID
        */
        bool IsSoundForComponentPlaying(uint64_t audioComponentID);

    private:
        friend class SourceManager;
        friend class Sound;
        friend class AudioPlayback;

        ma_engine mEngine;
        ma_log mmaLog;
        bool bInitialized = false;
        ma_sound mTestSound;

        Scope<Audio::DSP::Reverb> mMasterReverb = nullptr;

        SourceManager mSourceManager{ *this };

        AudioListener mAudioListener;
        Ref<Scene> mSceneContext;
        UUID mCurrentSceneID;

        /* Maximum number of sources availble due to platform limitation, or user settings.
            (for now this is just an arbitrary number)
        */
        int mNumSources = 0;
        std::vector<Sound*> mSoundSources;
        std::vector<Sound*> mActiveSounds;
        std::vector<Sound*> mSoundsToStart;

        ////EntityIDMap<Sound*> mComponentSoundMap;
        EntityIDMap<UUID> mComponentObjectMap;
        AudioComponentRegistry mAudioComponentRegistry;

        std::mutex mUpdateSourcesLock;
        std::vector<SoundSourceUpdateData> mSourceUpdateData;

        std::shared_mutex mObjectsLock;
        std::unordered_map<UUID, AudioObject> mAudioObjects;

        std::queue< std::pair<Audio::ECommandType, Audio::EventInfo> > mCommandQueue;

        // Map of Objects and associated active Sources
        Audio::ObjectSourceRegistry mObjectSourceMap;
        // Map of all active Events
        Audio::EventRegistry mEventRegistry;
        // Map of Events active on Objects
        Audio::ObjectEventRegistry mObjectEventsRegistry;

        //==============================================
        static AudioEngine* sInstance;

        static Audio::Stats sStats;
        Audio::AllocationCallbackData mEngineCallbackData{ false, sStats };
        Audio::AllocationCallbackData mRMCallbackData{ true, sStats };

    };
} // namespace