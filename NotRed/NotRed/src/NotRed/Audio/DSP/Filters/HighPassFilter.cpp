#include <nrpch.h>
#include "HighPassFilter.h"

namespace NR::Audio::DSP
{
    static void hpf_node_process_pcmframes(ma_node* pNode, const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut)
    {
        const float* pFramesIn_0 = ppFramesIn[0]; // Input bus @ index 0.
        const float* pFramesIn_1 = ppFramesIn[1]; // Input bus @ index 1.
        float* pFramesOut_0 = ppFramesOut[0];     // Output bus @ index 0.

        auto* node = static_cast<HighPassFilter::hpf_node*>(pNode);

        ma_hpf2_process_pcm_frames(&node->filter, pFramesOut_0, pFramesIn_0, *pFrameCountOut);
    }

    // TODO: use second input bus for Bypass functionality?
    //       Or it can be used for a modulator signal!
    static ma_node_vtable high_pass_filter_vtable = {
        hpf_node_process_pcmframes,
        nullptr,
        2, // 2 input buses.
        1, // 1 output bus.
        0 // Default flags.
    };


    //===========================================================================
    static ma_allocation_callbacks allocation_callbacks;

    HighPassFilter::HighPassFilter()
    {
    }

    HighPassFilter::~HighPassFilter()
    {
        Uninitialize();
    }

    void HighPassFilter::Uninitialize()
    {
        if (mInitialized)
        {
            if (((ma_node_base*)&mNode)->vtable != nullptr)
            {
                ma_node_uninit(&mNode, &allocation_callbacks);
            }
        }

        mInitialized = false;
    }

    bool HighPassFilter::Initialize(ma_engine* engine, ma_node_base* nodeToInsertAfter)
    {
        NR_CORE_ASSERT(!mInitialized);

        auto abortIfFailed = [&](ma_result result, const char* errorMessage)
            {
                if (result != MA_SUCCESS)
                {
                    NR_CORE_ASSERT(false && errorMessage);
                    Uninitialize();
                    return true;
                }

                return false;
            };

        mSampleRate = ma_engine_get_sample_rate(engine);;
        uint8_t numChannels = ma_node_get_output_channels(nodeToInsertAfter, 0);

        ma_uint32 inChannels[2]{ numChannels, numChannels };
        ma_uint32 outChannels[1]{ numChannels };
        ma_node_config nodeConfig = ma_node_config_init();
        nodeConfig.vtable = &high_pass_filter_vtable;
        nodeConfig.pInputChannels = inChannels;
        nodeConfig.pOutputChannels = outChannels;
        nodeConfig.initialState = ma_node_state_started;

        ma_result result;
        allocation_callbacks = engine->pResourceManager->config.allocationCallbacks;
        result = ma_node_init(&engine->nodeGraph, &nodeConfig, &allocation_callbacks, &mNode);
        if (abortIfFailed(result, "Node Init failed"))
        {
            return false;
        }

        ma_hpf2_config config = ma_hpf2_config_init(ma_format_f32, numChannels, (ma_uint32)mSampleRate, mCutoffMultiplier.load(), 0.707);
        result = ma_hpf2_init(&config, &mNode.filter);
        if (abortIfFailed(result, "Filter Init failed"))
        {
            return false;
        }

        auto* output = nodeToInsertAfter->pOutputBuses[0].pInputNode;

        // attach to the output of the node that this filter is connected to
        result = ma_node_attach_output_bus(&mNode, 0, output, 0);
        NR_CORE_ASSERT(result == MA_SUCCESS);
        if (abortIfFailed(result, "Node attach failed"))
        {
            return false;
        }

        // attach passed in node to the filter
        result = ma_node_attach_output_bus(nodeToInsertAfter, 0, &mNode, 0);
        if (abortIfFailed(result, "Node attach failed"))
            return false;

        mInitialized = true;

        return mInitialized;
    }

    void HighPassFilter::SetCutoffValue(double cutoffMultiplier)
    {
        double cutoffFrequency = cutoffMultiplier * 20000.0;
        ma_hpf2_config config = ma_hpf2_config_init(mNode.filter.bq.format, mNode.filter.bq.channels, (ma_uint32)mSampleRate, cutoffFrequency, 0.707);

        ma_hpf2_reinit(&config, &mNode.filter);
    }

    void HighPassFilter::SetParameter(uint8_t parameterIdx, float value)
    {
        if (parameterIdx == EHPFParameters::CutOffFrequency)
        {
            mCutoffMultiplier = (double)value;
            SetCutoffValue(mCutoffMultiplier.load()); // TODO: call this from update loop
        }
    }

    float HighPassFilter::GetParameter(uint8_t parameterIdx) const
    {
        if (parameterIdx == EHPFParameters::CutOffFrequency)
        {
            return (float)mCutoffMultiplier.load();
        }
        else
        {
            return -1.0f;
        }
    }

    const char* HighPassFilter::GetParameterLabel(uint8_t parameterIdx) const
    {
        if (parameterIdx == EHPFParameters::CutOffFrequency)
        {
            return "Hz";
        }
        else
        {
            return "Unkonw parameter index";
        }
    }

    std::string HighPassFilter::GetParameterDisplay(uint8_t parameterIdx) const
    {
        if (parameterIdx == EHPFParameters::CutOffFrequency)
            return std::to_string(mCutoffMultiplier.load());
        else
            return "Unkonw parameter index";
    }

    const char* HighPassFilter::GetParameterName(uint8_t parameterIdx) const
    {
        if (parameterIdx == EHPFParameters::CutOffFrequency)
            return "Cut-Off Frequency";
        else
            return "Unkonw parameter index";
    }

    uint8_t HighPassFilter::GetNumberOfParameters() const
    {
        return 1;
    }

}