#include <nrpch.h>
#include "LowPassFilter.h"

namespace NR::Audio::DSP
{
    static void lpf_node_process_pcmframes(ma_node* pNode, const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut)
    {
        const float* pFramesIn_0 = ppFramesIn[0]; // Input bus @ index 0.
        const float* pFramesIn_1 = ppFramesIn[1]; // Input bus @ index 1.
        float* pFramesOut_0 = ppFramesOut[0];     // Output bus @ index 0.

        auto* node = static_cast<LowPassFilter::lpf_node*>(pNode);

        ma_lpf1_process_pcm_frames(&node->filter, pFramesOut_0, pFramesIn_0, *pFrameCountOut);
    }

    // TODO: use second input bus for Bypass functionality?
    //       Or it can be used for a modulator signal!
    static ma_node_vtable low_pass_filter_vtable = {
        lpf_node_process_pcmframes,
        nullptr,
        2, // 2 input buses.
        1, // 1 output bus.
        0 // Default flags.
    };

    //===========================================================================
    static ma_allocation_callbacks allocation_callbacks;

    LowPassFilter::LowPassFilter()
    {
    }

    LowPassFilter::~LowPassFilter()
    {
        Uninitialize();
    }

    void LowPassFilter::Uninitialize()
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


    bool LowPassFilter::Initialize(ma_engine* engine, ma_node_base* nodeToInsertAfter)
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
        nodeConfig.vtable = &low_pass_filter_vtable;
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

        ma_lpf1_config config = ma_lpf1_config_init(ma_format_f32, numChannels, (ma_uint32)mSampleRate, mCutoffMultiplier.load());
        result = ma_lpf1_init(&config, &mNode.filter);
        if (abortIfFailed(result, "Filter Init failed"))
        {
            return false;
        }

        auto* output = nodeToInsertAfter->pOutputBuses[0].pInputNode;

        // attach to the output of the node that this filter is connected to
        result = ma_node_attach_output_bus(&mNode, 0, output, 0);
        if (abortIfFailed(result, "Node attach failed"))
        {
            return false;
        }

        // attach passed in node to the filter
        result = ma_node_attach_output_bus(nodeToInsertAfter, 0, &mNode, 0);
        if (abortIfFailed(result, "Node attach failed"))
        {
            return false;
        }

        mInitialized = true;

        return mInitialized;
    }

    // TODO: check for the "zipper" noise artefacts
    void LowPassFilter::SetCutoffValue(double cutofNormilized)
    {
        double cutoffFrequency = cutofNormilized * 20000.0;
        ma_lpf1_config config = ma_lpf1_config_init(mNode.filter.format, mNode.filter.channels, (ma_uint32)mSampleRate, cutoffFrequency);

        // TODO: make sure this thread safe, e.i. using atomic operation to update the values
        ma_lpf1_reinit(&config, &mNode.filter);
    }

    void LowPassFilter::SetParameter(uint8_t parameterIdx, float value)
    {
        if (parameterIdx == ELPFParameters::CutOffFrequency)
        {
            mCutoffMultiplier = (double)value;
            SetCutoffValue(mCutoffMultiplier.load()); // TODO: call this from update loop
        }
    }

    float LowPassFilter::GetParameter(uint8_t parameterIdx) const
    {
        if (parameterIdx == ELPFParameters::CutOffFrequency)
        {
            return (float)mCutoffMultiplier.load();
        }
        else
        {
            return -1.0f;
        }
    }

    const char* LowPassFilter::GetParameterLabel(uint8_t parameterIdx) const
    {
        if (parameterIdx == ELPFParameters::CutOffFrequency)
        {
            return "Hz";
        }
        else
        {
            return "Unkonw parameter index";
        }
    }

    std::string LowPassFilter::GetParameterDisplay(uint8_t parameterIdx) const
    {
        if (parameterIdx == ELPFParameters::CutOffFrequency)
        {
            return std::to_string(mCutoffMultiplier.load());
        }
        else
        {
            return "Unkonw parameter index";
        }
    }

    const char* LowPassFilter::GetParameterName(uint8_t parameterIdx) const
    {
        if (parameterIdx == ELPFParameters::CutOffFrequency)
        {
            return "Cut-Off Frequency";
        }
        else
        {
            return "Unkonw parameter index";
        }
    }

    uint8_t LowPassFilter::GetNumberOfParameters() const
    {
        return 1;
    }
}