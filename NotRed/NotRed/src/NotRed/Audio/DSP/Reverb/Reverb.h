#pragma once

#include "miniaudio.h"
#include "miniaudio_engine.h"

#include <memory>

class revmodel;

namespace NR::Audio::DSP
{
    class DelayLine;

    enum EReverbParameters
    {
        PreDelay, Mode, RoomSize, Damp, Width, Wet, Dry,
        NumParams
    };

    struct Reverb
    {
    public:
        Reverb();
        ~Reverb();

        bool Initialize(ma_engine* engine, ma_node_base* nodeToAttachTo);
        void Uninitialize();
        ma_node_base* GetNode() { return &mNode.base; }

        // TODO: put this into an EffectInterface
        void SetParameter(EReverbParameters parameter, float value);
        float GetParameter(EReverbParameters parameter) const;
        const char* GetParameterLabel(EReverbParameters parameter) const;
        std::string GetParameterDisplay(EReverbParameters parameter) const;
        const char* GetParameterName(EReverbParameters parameter) const;

    private:
        // --- Internal members
        bool mInitialized = false;

        friend void reverb_node_process_pcm_frames(ma_node* pNode, const float** ppFramesIn, ma_uint32* pFrameCountIn,
            float** ppFramesOut, ma_uint32* pFrameCountOut);

        const float mMaxPreDelay = 1000.0f;
        struct reverb_node
        {
            ma_node_base base; // <-- This must always the first member.
            DelayLine* delayLine;
            revmodel* reverb;
        };
        reverb_node mNode;
        // ~ End of internal members

        std::unique_ptr<revmodel> mRevModel = nullptr;
        std::unique_ptr<DelayLine> mDelayLine = nullptr;

        //=================================
        //? Currently not used.
        void Suspend();
        void Resume();
    };
}