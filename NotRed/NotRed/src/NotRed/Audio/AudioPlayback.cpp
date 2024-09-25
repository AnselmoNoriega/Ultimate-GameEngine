#include "nrpch.h"
#include "AudioPlayback.h"

#include "AudioEngine.h"
#include "AudioComponent.h"

#include "NotRed/Scene/Components.h"
#include "NotRed/Scene/Scene.h"
#include "NotRed/Scene/Entity.h"

namespace NR::Audio
{
    bool AudioPlayback::PlaySound2D(const SoundConfig& sound, float volume, float pitch)
    {
        auto& engine = MiniAudioEngine::Get();
        auto& scene = engine.GetCurrentSceneContext();
        auto entity = scene->CreateEntityWithID(UUID(), "OneShot2D");

        auto& audioComponent = entity.AddComponent<AudioComponent>(entity.GetID());
        audioComponent.SoundConfig = sound;
        audioComponent.SoundConfig.bSpatializationEnabled = false;
        audioComponent.VolumeMultiplier = volume;
        audioComponent.PitchMultiplier = pitch;
        audioComponent.AutoDestroy = true;

        uint64_t handle = entity.GetID();
        MiniAudioEngine::ExecuteOnAudioThread([handle, sound]
            {
                MiniAudioEngine::Get().SubmitSoundToPlay(handle, sound);
            }, "PlaySound2D()");

        return true;
    }

    bool AudioPlayback::PlaySound2D(Ref<Asset> soundAsset, float volume, float pitch)
    {
        SoundConfig sc;
        sc.FileAsset = soundAsset;
        return PlaySound2D(sc, volume, pitch);
    }

    bool AudioPlayback::PlaySoundAtLocation(const SoundConfig& sound, glm::vec3 location, float volume, float pitch)
    {
        auto& engine = MiniAudioEngine::Get();
        auto& scene = engine.GetCurrentSceneContext();
        auto entity = scene->CreateEntityWithID(UUID(), "OneShot3D");

        entity.GetComponent<TransformComponent>().Translation = location;

        auto& audioComponent = entity.AddComponent<AudioComponent>(entity.GetID());
        audioComponent.SoundConfig = sound;
        audioComponent.SoundConfig.SpawnLocation = location;
        audioComponent.SoundConfig.bSpatializationEnabled = true;
        audioComponent.VolumeMultiplier = volume;
        audioComponent.PitchMultiplier = pitch;
        audioComponent.SourcePosition = location;
        audioComponent.AutoDestroy = true;
        SoundConfig config = audioComponent.SoundConfig;
        uint64_t handle = entity.GetID();
        MiniAudioEngine::ExecuteOnAudioThread([handle, config]
            {
                MiniAudioEngine::Get().SubmitSoundToPlay(handle, config);
            }, "PlaySoundAtLocation()");
        return true;
    }

    bool AudioPlayback::PlaySoundAtLocation(Ref<Asset> soundAsset, glm::vec3 location, float volume, float pitch)
    {
        SoundConfig sc;
        sc.FileAsset = soundAsset;
        return PlaySoundAtLocation(sc, location, volume, pitch);
    }

    bool AudioPlayback::Play(uint64_t audioComponentID, float startTime)
    {
        auto& engine = MiniAudioEngine::Get();
        return engine.SubmitSoundToPlay(audioComponentID);
    }

    bool AudioPlayback::StopActiveSound(uint64_t audioComponentID)
    {
        auto& engine = MiniAudioEngine::Get();
        return engine.StopActiveSoundSource(audioComponentID);
    }

    bool AudioPlayback::PauseActiveSound(uint64_t audioComponentID)
    {
        auto& engine = MiniAudioEngine::Get();
        return engine.PauseActiveSoundSource(audioComponentID);
    }

    bool AudioPlayback::IsPlaying(uint64_t audioComponentID)
    {
        auto& engine = MiniAudioEngine::Get();
        return engine.IsSoundForComponentPlaying(audioComponentID);
    }
}