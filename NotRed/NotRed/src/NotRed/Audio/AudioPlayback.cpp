#include "nrpch.h"
#include "AudioPlayback.h"

#include "AudioEngine.h"
#include "AudioComponent.h"
#include "SourceManager.h"

#include "NotRed/Scene/Components.h"
#include "NotRed/Scene/Scene.h"
#include "NotRed/Scene/Entity.h"

#include "NotRed/Debug/Profiler.h"

namespace NR::Audio
{
    bool AudioPlayback::PlaySound2D(const SoundConfig& sound, float volume, float pitch)
    {
        NR_PROFILE_FUNC();

        auto& engine = AudioEngine::Get();
        auto& scene = engine.GetCurrentSceneContext();
        auto entity = scene->CreateEntityWithID(UUID(), "OneShot2D");

        auto& audioComponent = entity.AddComponent<AudioComponent>(entity.GetID());
        audioComponent.SoundConfig = sound;
        audioComponent.SoundConfig.SpatializationEnabled = false;
        audioComponent.VolumeMultiplier = volume;
        audioComponent.PitchMultiplier = pitch;
        audioComponent.AutoDestroy = true;

        uint64_t handle = entity.GetID();
        AudioEngine::ExecuteOnAudioThread([handle, sound]
            {
                AudioEngine::Get().SubmitSoundToPlay(handle, sound);
            }, "PlaySound2D()");

        return true;
    }

    bool AudioPlayback::PlaySound2D(Ref<AudioFile> soundAsset, float volume, float pitch)
    {
        NR_PROFILE_FUNC();

        SoundConfig sc;
        sc.FileAsset = soundAsset;
        return PlaySound2D(sc, volume, pitch);
    }

    bool AudioPlayback::PlaySoundAtLocation(const SoundConfig& sound, glm::vec3 location, float volume, float pitch)
    {
        NR_PROFILE_FUNC();

        auto& engine = AudioEngine::Get();
        auto& scene = engine.GetCurrentSceneContext();
        auto entity = scene->CreateEntityWithID(UUID(), "OneShot3D");

        entity.GetComponent<TransformComponent>().Translation = location;

        auto& audioComponent = entity.AddComponent<AudioComponent>(entity.GetID());
        audioComponent.SoundConfig = sound;
        audioComponent.SoundConfig.SpawnLocation = location;
        audioComponent.SoundConfig.SpatializationEnabled = true;
        audioComponent.VolumeMultiplier = volume;
        audioComponent.PitchMultiplier = pitch;
        audioComponent.SourcePosition = location;
        audioComponent.AutoDestroy = true;

        SoundConfig config = audioComponent.SoundConfig;
        uint64_t handle = entity.GetID();
        AudioEngine::ExecuteOnAudioThread([handle, config]
            {
                AudioEngine::Get().SubmitSoundToPlay(handle, config);
            }, "PlaySoundAtLocation()");
        return true;
    }

    bool AudioPlayback::PlaySoundAtLocation(Ref<AudioFile> soundAsset, glm::vec3 location, float volume, float pitch)
    {
        NR_PROFILE_FUNC();

        SoundConfig sc;
        sc.FileAsset = soundAsset;
        return PlaySoundAtLocation(sc, location, volume, pitch);
    }

    bool AudioPlayback::Play(uint64_t audioComponentID, float startTime)
    {
        NR_PROFILE_FUNC();

        auto& engine = AudioEngine::Get();
        return engine.SubmitSoundToPlay(audioComponentID);
    }

    bool AudioPlayback::StopActiveSound(uint64_t audioComponentID)
    {
        NR_PROFILE_FUNC();

        auto& engine = AudioEngine::Get();
        return engine.StopActiveSoundSource(audioComponentID);
    }

    bool AudioPlayback::PauseActiveSound(uint64_t audioComponentID)
    {
        NR_PROFILE_FUNC();

        auto& engine = AudioEngine::Get();
        return engine.PauseActiveSoundSource(audioComponentID);
    }

    bool AudioPlayback::IsPlaying(uint64_t audioComponentID)
    {
        NR_PROFILE_FUNC();

        auto& engine = AudioEngine::Get();
        return engine.IsSoundForComponentPlaying(audioComponentID);
    }

    void AudioPlayback::SetMasterReverbSend(uint64_t audioComponetnID, float sendLevel)
    {
        NR_PROFILE_FUNC();

        auto& engine = AudioEngine::Get();
        if (auto* sound = engine.GetSoundForAudioComponent(audioComponetnID))
        {
            SourceManager::SetMasterReverbSendForSource(sound, sendLevel);
            if (auto* audioComponent = engine.GetAudioComponentFromID(engine.mCurrentSceneID, audioComponetnID))
            {
                audioComponent->SoundConfig.MasterReverbSend = sendLevel;
            }
        }
    }

    float AudioPlayback::GetMasterReverbSend(uint64_t audioComponetnID)
    {
        NR_PROFILE_FUNC();

        auto& engine = AudioEngine::Get();
        if (auto* audioComponent = engine.GetAudioComponentFromID(engine.mCurrentSceneID, audioComponetnID))
        {
            return audioComponent->SoundConfig.MasterReverbSend;
        }

        return -1.0f;
    }
}