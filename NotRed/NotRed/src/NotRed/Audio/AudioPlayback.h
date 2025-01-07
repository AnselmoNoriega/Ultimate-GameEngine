#pragma once

#include <optional>

#include "Audio.h"
#include "Sound.h"
#include "AudioEvents/AudioCommands.h"
#include "AudioEvents/CommandID.h"

namespace NR
{
    /*  ========================================
        Audio Playback interface

         In the future this might be be replaced
         with Audio Events Abstraction Layer
        ---------------------------------------
    */
    class AudioPlayback
    {
    public:

        // Playback of sound source associated to an existing AudioComponent
        static bool Play(uint64_t audioComponentID, float startTime = 0.0f/*, float fadeInTime = 0.0f*/); // TODO: fadeInTime
        static bool StopActiveSound(uint64_t audioComponentID);
        static bool PauseActiveSound(uint64_t audioComponentID);
        static bool Resume(uint64_t audioComponentID);
        static bool IsPlaying(uint64_t audioComponentID);

        //==================================
        //=== Audio Events Abstraction Layer 

        static uint32_t PostTrigger(Audio::CommandID triggerCommandID, uint64_t audioObjectID);
        static uint32_t PostTriggerFromAC(Audio::CommandID triggerCommandID, uint64_t audioComponentID);

        // Handy function to post trigger event at location without having to manually initialize and release an AudioObject
        static uint32_t PostTriggerAtLocation(Audio::CommandID triggerID, const Audio::Transform& location);

        static uint64_t InitializeAudioObject(const std::string& debugName, const Audio::Transform& objectPosition);
        static void ReleaseAudioObject(uint64_t objectID);

        // @return debug name if object found.
        static std::optional<std::string> FindObject(uint64_t objectID);

        static bool StopEventID(uint32_t playingEvent);
        static bool PauseEventID(uint32_t playingEvent);
        static bool ResumeEventID(uint32_t playingEvent);

    };
}