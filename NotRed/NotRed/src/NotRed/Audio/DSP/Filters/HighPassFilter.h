#pragma once

#include "MiniAudio/include/miniaudioInc.h"

namespace NR::Audio::DSP
{
    struct HighPassFilter
    {
    public:
        HighPassFilter();
        ~HighPassFilter();

        enum EHPFParameters : uint8_t
        {
            CutOffFrequency
        };

        bool Initialize(ma_engine* engine, ma_node_base* nodeToInsertAfter);
        void Uninitialize();
        void SetCutoffValue(double cutoffMultiplier);

        ma_node_base* GetNode() { return &mNode.base; }

        void SetParameter(uint8_t parameterIdx, float value);
        float GetParameter(uint8_t parameterIdx) const;
        const char* GetParameterLabel(uint8_t parameterIdx) const;
        std::string GetParameterDisplay(uint8_t parameterIdx) const;
        const char* GetParameterName(uint8_t parameterIdx) const;
        uint8_t GetNumberOfParameters() const;

    private:
        // --- Internal members
        bool mInitialized = false;

        friend void hpf_node_process_pcmframes(
            ma_node* pNode, 
            const float** ppFramesIn, 
            ma_uint32* pFrameCountIn,
            float** ppFramesOut, 
            ma_uint32* pFrameCountOut
        );

        struct hpf_node
        {
            ma_node_base base; // <-- Make sure this is always the first member.
            ma_hpf2 filter;
        };

        hpf_node mNode;
        double mSampleRate = 0.0;
        // ~ End of internal members

        std::atomic<double> mCutoffMultiplier = 0.0;
    };
}