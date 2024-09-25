#pragma once

#include <optional>
#include <queue>
#include <shared_mutex>
#include <thread>

#include <glm/glm.hpp>

#include "miniaudioInc.h"

#include "NotRed/Scene/Components.h"
#include "Audio.h"
#include "SourceManager.h"
#include "Sound.h"
#include "AudioComponent.h"
#include "AudioPlayback.h"

#include "NotRed/Core/Timer.h"
#include "NotRed/Scene/Scene.h"
#include "NotRed/Scene/Entity.h"

namespace NR::Audio
{

    class SourceManager;
    class Sound;
    class SoundSource;
    class AudioComponent;


    struct Stats
    {
        uint32_t NumActiveSounds = 0;
        uint32_t TotalSources = 0;
        uint64_t MemEngine = 0;
        uint64_t MemResManager = 0;
        float FrameTime = 0.0f;
        uint64_t NumAudioComps = 0;
    };

    struct AllocationCallbackData
    {
        bool isResourceManager = false;
        Stats& Stats;
    };

    struct AudioListener
    {
        bool PositionNeedsUpdate(const glm::vec3& newPosition, const glm::vec3& newDirection) const
        {
            std::shared_lock lock{ mMutex };
            return newPosition != mLastPosition || newDirection != mLastDirection;
        }

        void SetPositionDirection(const glm::vec3& newPosition, const glm::vec3& newDirection)
        {
            std::unique_lock lock{ mMutex };
            mLastPosition = newPosition;
            mLastDirection = newDirection;
            mChanged = true;
        }

        void SetVelocity(const glm::vec3& newVelocity)
        {
            std::unique_lock lock{ mMutex };
            mChanged = newVelocity != mLastVelocity;
            mLastVelocity = newVelocity;
        }

        void GetVelocity(glm::vec3& velocity) const
        {
            std::shared_lock lock{ mMutex };
            velocity = mLastVelocity;
        }

        void GetPositionDirection(glm::vec3& position, glm::vec3& direction) const
        {
            std::shared_lock lock{ mMutex };
            position = mLastPosition;
            direction = mLastDirection;
        }

        bool HasChanged(bool resetFlag) const
        {
            bool changed = mChanged.load();
            if (resetFlag) mChanged.store(false);
            {
                return changed;
            }
        }

    private:
        mutable std::shared_mutex mMutex;

        glm::vec3 mLastPosition = glm::vec3(0.0f);
        glm::vec3 mLastDirection = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 mLastVelocity = glm::vec3(0.0f);

        mutable std::atomic<bool> mChanged = false;
    };


    template<typename T>
    struct EntityIDMap
    {
        void Add(UUID sceneID, UUID entityID, T object)
        {
            std::scoped_lock lock{ mMutex };
            mEntityIDMap[sceneID][entityID] = object;
        }

        void Remove(UUID sceneID, UUID entityID)
        {
            std::scoped_lock lock{ mMutex };
            NR_CORE_ASSERT(mEntityIDMap.at(sceneID).find(entityID) != mEntityIDMap.at(sceneID).end(), "Could not find entityID in the registry to remove.");

            mEntityIDMap.at(sceneID).erase(entityID);
            if (mEntityIDMap.at(sceneID).empty())
            {
                mEntityIDMap.erase(sceneID);
            }
        }

        std::optional<T> Get(UUID sceneID, UUID entityID) const
        {
            std::shared_lock lock{ mMutex };
            if (mEntityIDMap.find(sceneID) != mEntityIDMap.end())
            {
                if (mEntityIDMap.at(sceneID).find(entityID) != mEntityIDMap.at(sceneID).end())
                {
                    return std::optional<T>(mEntityIDMap.at(sceneID).at(entityID));
                }
            }
            return std::optional<T>();
        }

        void Clear(UUID sceneID)
        {
            std::scoped_lock lock{ mMutex };
            if (sceneID == 0)
            {
                mEntityIDMap.clear();
            }
            else if (mEntityIDMap.find(sceneID) != mEntityIDMap.end())
            {
                mEntityIDMap.at(sceneID).clear();
                mEntityIDMap.erase(sceneID);
            }
        }

        uint64_t Count(UUID sceneID) const
        {
            std::shared_lock lock{ mMutex };
            if (mEntityIDMap.find(sceneID) != mEntityIDMap.end())
                return mEntityIDMap.at(sceneID).size();
            else
                return 0;
        }

    private:
        mutable std::shared_mutex mMutex;
        std::unordered_map<UUID, std::unordered_map<UUID, T>> mEntityIDMap;
    };

    struct AudioComponentRegistry : public EntityIDMap<Entity>
    {
        AudioComponent* GetAudioComponent(UUID sceneID, uint64_t audioComponentID) const
        {
            std::optional<Entity> o = Get(sceneID, audioComponentID);
            if (o.has_value())
            {
                return &o->GetComponent<AudioComponent>();
            }

            else
            {
               NR_CORE_ASSERT("Component was not found in registry");
               return nullptr;
            }
        }

        Entity GetEntity(UUID sceneID, uint64_t audioComponentID) const
        {
            return Get(sceneID, UUID(audioComponentID)).value_or(Entity());
        }
    };

    class MiniAudioEngine
    {
    public:
        MiniAudioEngine();
        ~MiniAudioEngine();

        static void Init();

        static void Shutdown();

        static inline MiniAudioEngine& Get() { return *sInstance; }

        static void SetSceneContext(const Ref<Scene>& scene);
        static Ref<Scene>& GetCurrentSceneContext();
        static void OnRuntimePlaying(UUID sceneID);
        static void OnSceneDestruct(UUID sceneID);

        static Stats GetStats();

        bool Initialize();

        bool Uninitialize();

        /* Stop immediately if set 'true', otherwise apply "stop-fade"*/
        void StopAll(bool stopNow = false);
        static void StopAllSounds(bool stopNow = false) { sInstance->StopAll(stopNow); }

        SourceManager& GetSourceManager() { return mSourceManager; }
        const SourceManager& GetSourceManager() const { return mSourceManager; }

        /* Main Audio Thread tick update function */
        void Update(TimeFrame dt);

        void SubmitSourceUpdateData(std::vector<SoundSourceUpdateData> updateData);

        /* Update positions of sound sources from game Entities.*/
        void UpdateSoundPositions(std::vector<std::pair<uint64_t, glm::vec3>>& positions);

        /* Update velocities of sound sources from game Entities.*/
        void UpdateSoundVelocities(std::vector<std::pair<uint64_t, glm::vec3>>& velocities);

        /* Update Audio Listener position from game Entity owning active AudioListenerComponent.
            Called from Game Thread.*/
        void UpdateListenerPosition(const glm::vec3& newTranslation, const glm::vec3& newDirection);

        /* Update Audio Listener velocity from game Entity owning active AudioListenerComponent.
            Called from Game Thread.*/
        void UpdateListenerVelocity(const glm::vec3& newVelocity);

        void RegisterNewListener(AudioListenerComponent& listenerComponent);


        /* Execute arbitrary function on Audio Thread. Used to synchronize updates between Game Thread and Audio Thread */
        static void ExecuteOnAudioThread(AudioThreadCallbackFunction func, /*void* parameter, */const char* jobID = "NONE");

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

        void UpdateSources();

        /* This attempts to find ActiveSound for the AudioComponent.

            @returns nullptr - if there is no ActiveSound associated to supplied audioComponentID
        */
        Sound* GetSoundForAudioComponent(uint64_t audioComponentID);

        /* This attempts to find ActiveSound for the AudioComponent first. If failed,
            it attempts to initialize new sound source. When the sound source is obtaint,
            it is added to ActiveSounds list.

            @param audioComponentID - AudioComponent to get sound for
            @param sourceConfig - configuration to initialize new sound source from

           @returns nullptr - if there is no ActiveSound associated to supplied audioComponentID
        */
        Sound* GetSoundForAudioComponent(uint64_t audioComponentID, const SoundConfig& sourceConfig);

        /* This is called when there is no free source available in pool for new playback start request. */
        Sound* FreeLowestPrioritySource();


        //==================================================================================
        //=== Playback interface. In most cases these functions are called from Game Thread.

        /*  Internal version of "Play" command. Attempts to GetSoundForAudioComponent()
            and submit it for the playback.

            @param audioComponentID - AudioComponent to associate sound with
            @param sourceConfig - configuration to initialize new sound source from
        */
        void SubmitSoundToPlay(uint64_t audioComponentID, const SoundConfig& sourceConfig);

        /*  Internal version of "Play" command. Attempts to GetSoundForAudioComponent()
            and submit it for the playback. SoundConfig is taken from the AudioComponent.

            @param audioComponentID - AudioComponent to associate new sound with
            @returns false - if failed to find audioComponentID in AudioComponentRegistry
        */
        bool SubmitSoundToPlay(uint64_t audioComponentID);

        /*  Internal version of "Stop" command. Attempts to stop ActiveSound
            associated with audioComponentID.

            @returns false - if failed to find audioComponentID in AudioComponentRegistry
        */
        bool StopActiveSoundSource(uint64_t audioComponentID);

        /*  Internal version of "Pause" command. Attempts to pause ActiveSound
            associated with audioComponentID.

            @returns false - if failed to find audioComponentID in AudioComponentRegistry
        */
        bool PauseActiveSoundSource(uint64_t audioComponentID);

        /*  Check if AudioComponent has associated ActiveSound that's currently playing

            @returns false - if there is no ActiveSound associated to the audioComponentID
        */
        bool IsSoundForComponentPlaying(uint64_t audioComponentID);

    private:
        friend class SourceManager;
        friend class Sound;
        friend class AudioPlayback;

        ma_engine mEngine;
        bool bInitialized = false;
        ma_sound mTestSound;

        SourceManager mSourceManager{ *this };

        AudioListener mAudioListener;
        Ref<Scene> mSceneContext;
        UUID mCurrentSceneID;

        /* Maximum number of sources availble due to platform limitation, or user settings.
            (for now this is just an arbitrary number)
        */
        int mNumSources = 0;
        std::vector<Sound*> mSoundSources;
        std::vector<SoundObject*> mActiveSounds;
        std::vector<SoundObject*> mSoundsToStart;

        EntityIDMap<Sound*> mComponentSoundMap;
        AudioComponentRegistry mAudioComponentRegistry;

        std::mutex mUpdateSourcesLock;
        std::vector<SoundSourceUpdateData> mSourceUpdateData;

        //==============================================
        static MiniAudioEngine* sInstance;

        static Stats sStats;
        AllocationCallbackData mEngineCallbackData{ false, sStats };
        AllocationCallbackData mRMCallbackData{ true, sStats };
    };
} 