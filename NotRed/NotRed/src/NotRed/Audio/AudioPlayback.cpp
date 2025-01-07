#include <nrpch.h>
#include "AudioPlayback.h"

#include "AudioEngine.h"
#include "SourceManager.h"
#include "AudioComponent.h"
#include "NotRed/Scene/Components.h"

#include "NotRed/Audio/AudioEvents/AudioCommandRegistry.h"

#include "NotRed/Scene/Scene.h"
#include "NotRed/Scene/Entity.h"

#include "NotRed/Debug/Profiler.h"

namespace NR
{
    using namespace Audio;

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

    bool AudioPlayback::Resume(uint64_t audioComponentID)
    {
        NR_PROFILE_FUNC();

        auto& engine = AudioEngine::Get();
        return engine.ResumeActiveSoundSource(audioComponentID);
    }

    bool AudioPlayback::IsPlaying(uint64_t audioComponentID)
    {
        NR_PROFILE_FUNC();

        auto& engine = AudioEngine::Get();
        return engine.IsSoundForComponentPlaying(audioComponentID);
    }

    uint32_t AudioPlayback::PostTrigger(CommandID triggerCommandID, uint64_t audioObjectID)
    {
        if (AudioCommandRegistry::DoesCommandExist<TriggerCommand>(triggerCommandID))
        {
            auto& engine = AudioEngine::Get();

            return engine.PostTrigger(triggerCommandID, audioObjectID);
        }

        NR_CORE_ERROR("Audio command with ID {0} does not exist!", (int)triggerCommandID);
        return 0;
    }

    uint32_t AudioPlayback::PostTriggerFromAC(Audio::CommandID triggerCommandID, uint64_t audioComponentID)
    {
        if (AudioCommandRegistry::DoesCommandExist<TriggerCommand>(triggerCommandID))
        {
            auto& engine = AudioEngine::Get();

            UUID objectID = engine.GetAudioObjectOfComponent(audioComponentID);

            if (objectID == 0)
            {
                NR_CORE_ERROR("Object for AudioComponent ID {0} does not exist!", audioComponentID);
                return 0;
            }

            return engine.PostTrigger(triggerCommandID, objectID);
        }

        NR_CORE_ERROR("Audio command with ID {0} does not exist!", triggerCommandID);
        return 0;
    }

    uint32_t AudioPlayback::PostTriggerAtLocation(Audio::CommandID triggerID, const Audio::Transform& location)
    {
        if (AudioCommandRegistry::DoesCommandExist<TriggerCommand>(triggerID))
        {
            auto& engine = AudioEngine::Get();

            const auto objectID = UUID();
            engine.InitializeAudioObject(objectID, "One-Shot 3D", location);

            const uint32_t eventID = engine.PostTrigger(triggerID, objectID);

            engine.ReleaseAudioObject(objectID);

            return eventID;
        }

        return 0;
    }

    uint64_t AudioPlayback::InitializeAudioObject(const std::string& debugName, const Transform& objectPosition)
    {
        auto& engine = AudioEngine::Get();
        return engine.InitializeAudioObject(UUID(), debugName, objectPosition);
    }

    void AudioPlayback::ReleaseAudioObject(uint64_t objectID)
    {
        auto& engine = AudioEngine::Get();
        engine.ReleaseAudioObject(objectID);
    }

    std::optional<std::string> AudioPlayback::FindObject(uint64_t objectID)
    {
        auto& engine = AudioEngine::Get();
        if (auto* object = engine.GetAudioObject(objectID).value_or(nullptr))
        {
            return object->GetDebugName();
        }
        return std::optional<std::string>();
    }

    bool AudioPlayback::StopEventID(uint32_t playingEvent)
    {
        auto& engine = AudioEngine::Get();
        return engine.StopEventID(playingEvent);
    }

    bool AudioPlayback::PauseEventID(uint32_t playingEvent)
    {
        auto& engine = AudioEngine::Get();
        return engine.PauseEventID(playingEvent);
    }

    bool AudioPlayback::ResumeEventID(uint32_t playingEvent)
    {
        auto& engine = AudioEngine::Get();
        return engine.ResumeEventID(playingEvent);
    }
}