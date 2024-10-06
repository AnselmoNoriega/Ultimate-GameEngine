#pragma once

#include <memory>

#include "miniaudio.h"

#include "NotRed/Audio/DSP/Components/revmodel.hpp"
#include "NotRed/Audio/DSP/Components/DelayLine.h"

class revmodel;

namespace NR::Audio
{
    namespace DSP
    {
        enum EReverbParameters
        {
            PreDelay, Mode, RoomSize, Damp, Width, Wet, Dry,
            NumParams
        };

        struct Reverb
        {
        public:
            Reverb() = default;
            ~Reverb();

            bool Initialize(ma_engine* engine, ma_node_base* nodeToAttachTo);

            void SetParameter(EReverbParameters parameter, float value);

            ma_node_base* GetNode() { return &mNode.base; }

            float GetParameter(EReverbParameters parameter);
            const char* GetParameterName(EReverbParameters parameter);
            const char* GetParameterLabel(EReverbParameters parameter);
            const std::string& GetParameterDisplay(EReverbParameters parameter);

        private:
            friend void reverb_node_process_pcm_frames(ma_node* pNode, const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut);

            struct reverb_node
            {
                ma_node_base base; // <-- This must always be the first member.
                DelayLine* delayLine;
                revmodel* reverb;
            };

        private:
            void Suspend();
            void Resume();

        private:
            const float mMaxPreDelay = 1000.0f;

            reverb_node mNode;

            std::unique_ptr<revmodel> mRevModel = nullptr;
            std::unique_ptr<DelayLine> mDelayLine = nullptr;
        };
    }
}