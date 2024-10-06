#pragma once

#include "miniaudioInc.h"

namespace NR::Audio
{
    namespace DSP
    {
        struct LowPassFilter
        {
        public:
            LowPassFilter() = default;
            ~LowPassFilter();

            enum ELPFParameters : uint8_t
            {
                CutOffFrequency
            };

            bool Initialize(ma_engine* engine, ma_node_base* nodeToInsertAfter);

            void SetCutoffValue(double cutoffMultiplier);

            void SetParameter(uint8_t parameterIdx, float value);
            float GetParameter(uint8_t parameterIdx);
            const char* GetParameterLabel(uint8_t parameterIdx);
            const std::string& GetParameterDisplay(uint8_t parameterIdx);
            const char* GetParameterName(uint8_t parameterIdx);
            uint8_t GetNumberOfParameters();

            ma_node_base* GetNode() { return &mNode.base; }

        private:
            struct lpf_node
            {
                ma_node_base base; // <-- Make sure this is always the first member.
                ma_lpf1 filter;
            };

            friend void lpf_node_process_pcm_frames(ma_node* pNode, const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut);
            
        private:
            lpf_node mNode;
            double mSampleRate = 0.0;

            std::atomic<double> mCutoffMultiplier = 20000.0;
        };
    }
}