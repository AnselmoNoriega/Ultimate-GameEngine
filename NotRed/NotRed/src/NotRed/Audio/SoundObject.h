#pragma once
#include "NotRed/Core/TimeFrame.h"

namespace NR::Audio
{
    class SoundObject
    {
    public:
        virtual ~SoundObject() = default;

        virtual bool Play() = 0;
        virtual bool Stop() = 0;
        virtual bool Pause() = 0;

        virtual bool IsPlaying() const = 0;

        virtual void SetVolume(float newVolume) = 0;
        virtual void SetPitch(float newPitch) = 0;

        virtual float GetVolume() = 0;
        virtual float GetPitch() = 0;

        virtual bool FadeIn(const float duration, const float targetVolume) = 0;
        virtual bool FadeOut(const float duration, const float targetVolume) = 0;

        virtual void Update(float dt) = 0;
    };
}