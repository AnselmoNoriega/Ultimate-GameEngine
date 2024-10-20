#pragma once

#include "miniaudioInc.h"

namespace NR::Audio
{
    namespace DSP
    {
        class HighPassFilter
        {
        public:
            HighPassFilter() = default;
            ~HighPassFilter();

            enum EHPFParameters : uint8_t
            {
                CutOffFrequency
            };

            bool Initialize(ma_engine* engine, ma_node_base* nodeToInsertAfter);

            void SetCutoffValue(double cutoffMultiplier);

            void SetParameter(uint8_t parameterIdx, float value);
            float GetParameter(uint8_t parameterIdx) const;
            const char* GetParameterLabel(uint8_t parameterIdx) const;
            std::string GetParameterDisplay(uint8_t parameterIdx) const;
            const char* GetParameterName(uint8_t parameterIdx) const;
            uint8_t GetNumberOfParameters() const;

            ma_node_base* GetNode() { return &mNode.base; }

        private:
            struct hpf_node
            {
                ma_node_base base; // <-- Make sure this is always the first member.
                ma_hpf2 filter;
            };

            friend void hpf_node_process_pcm_frames(ma_node* pNode, const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut);

        private:
            hpf_node mNode;
            double mSampleRate = 0.0;

            std::atomic<double> mCutoffMultiplier = 0.0;
        };
    }
}