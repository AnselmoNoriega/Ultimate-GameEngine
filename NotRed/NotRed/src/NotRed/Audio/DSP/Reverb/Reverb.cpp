#include "nrpch.h"
#include "Reverb.h"

#include "NotRed/Audio/DSP/Components/revmodel.hpp"
#include "NotRed/Audio/DSP/Components/DelayLine.h"

namespace NR::Audio
{
    namespace DSP
    {
        static void reverb_node_process_pcm_frames(ma_node* pNode, const float** ppFramesIn, ma_uint32* pFrameCountIn, float** ppFramesOut, ma_uint32* pFrameCountOut)
        {
            Reverb::reverb_node* node = static_cast<Reverb::reverb_node*>(pNode);
            ma_node_base* pNodeBase = &node->base;
            ma_uint32 outBuses = ma_node_get_output_bus_count(pNodeBase);

            if (ppFramesIn == nullptr)
            {
                for (int oBus = 0; oBus < outBuses; ++oBus)
                {
                    uint32_t numChannels = ma_node_get_output_channels(pNodeBase, oBus);
                    size_t frameSize = ma_get_bytes_per_frame(ma_format_f32, numChannels);
                    memset(ppFramesOut[oBus], 0, frameSize * (*pFrameCountOut));
                }

                return;
            }

            const float* pFramesIn_0 = ppFramesIn[0];   // Input bus @ index 0.
            const float* pFramesIn_1 = ppFramesIn[1];   // Input bus @ index 1.
            float* pFramesOut_0 = ppFramesOut[0];       // Output bus @ index 0.

            /* NOTE: This assumes the same number of channels for all inputs and outputs. */
            ma_uint32 channels;
            channels = ma_node_get_input_channels(pNodeBase, 0);
            NR_CORE_ASSERT(channels == 2);

            // 1. Feed Pre-Delay
            auto* delay = node->delayLine;

            const float* channelDataL = &pFramesIn_0[0];
            const float* channelDataR = &pFramesIn_0[1];

            float* outputL = &pFramesOut_0[0];
            float* outputR = &pFramesOut_0[1];

            const float wetMix = 1.0f;
            const float dryMix = 1.0f - wetMix;
            const float feedback = 0.0f;

            int numsamples = *pFrameCountIn;

            while (numsamples-- > 0)
            {
                float delayedSampleL = delay->PopSample(0);
                float delayedSampleR = delay->PopSample(1);

                float outputSampleL = (*channelDataL * dryMix + (wetMix * delayedSampleL));
                float outputSampleR = (*channelDataR * dryMix + (wetMix * delayedSampleR));

                delay->PushSample(0, *channelDataL + (delayedSampleL * feedback));
                delay->PushSample(1, *channelDataR + (delayedSampleR * feedback));

                // Assign output sample computed above to the output buffer
                *outputL = outputSampleL;
                *outputR = outputSampleR;

                channelDataL += channels;
                channelDataR += channels;

                outputL += channels;
                outputR += channels;
            }

            // 2. Process delayed signal with reverb
            node->reverb->processreplace(pFramesOut_0, &pFramesOut_0[1], pFramesOut_0, &pFramesOut_0[1], *pFrameCountIn, channels);
        }

        static ma_node_vtable reverb_vtable = {
            reverb_node_process_pcmframes,
            nullptr,
            2, // 1 input bus.
            1, // 1 output bus.
            MA_NODE_FLAG_CONTINUOUS_PROCESSING | MA_NODE_FLAG_ALLOW_NULL_INPUT
        };

        //===========================================================================
        Reverb::~Reverb()
        {
            ma_node_uninit(&mNode, nullptr);
        }

        bool Reverb::InitNode(ma_node_graph* nodeGraph, double sampleRate)
        {
            // Setting max pre-delay time to 1 second
            mDelayLine = std::make_unique<DelayLine>(sampleRate + 1);
            mRevModel = std::make_unique<revmodel>();

            uint32_t inputChannels[2] = { 2, 2 };
            uint32_t outputChannels[1] = { 2 };

            ma_node_config nodeConfig = ma_node_config_init();
            nodeConfig.vtable = &reverb_vtable;
            nodeConfig.pInputChannels = inputChannels;
            nodeConfig.pOutputChannels = outputChannels;
            nodeConfig.initialState = ma_node_state_started;

            ma_result result;
            result = ma_node_init(nodeGraph, &nodeConfig, NULL, &mNode);
            if (result != MA_SUCCESS)
            {
                NR_CORE_ASSERT(false, "Node Init failed");
            }

            mDelayLine->SetConfig(ma_node_get_input_channels(&mNode, 0), sampleRate);
            
            // Set default pre-delay time to 50ms
            mDelayLine->SetDelayMs(50);
            mNode.delayLine = mDelayLine.get();
            mNode.reverb = mRevModel.get();

            result = ma_node_attach_output_bus(&mNode, 0, &nodeGraph->endpoint, 0);
            return result == MA_SUCCESS;
        }

        void Reverb::Suspend()
        {
            mRevModel->mute();
        }

        void Reverb::Resume()
        {
            mRevModel->mute();
        }

        void Reverb::SetParameter(EReverbParameters parameter, float value)
        {
            switch (parameter)
            {
            case PreDelay:
            {
                NR_CORE_ASSERT(value <= mMaxPreDelay);
                mDelayLine->SetDelayMs((uint32_t)value);
                break;
            }
            case Mode:
            {
                mRevModel->setmode(value);
                break;
            }
            case RoomSize:
            {
                mRevModel->setroomsize(value); //the roomsize must be less than 1.0714, and so the GUI slider max should be 1 (f=0.98)
                break;
            }
            case Damp:
            {
                mRevModel->setdamp(value);
                break;
            }
            case Wet:
            {
                mRevModel->setwet(value);
                break;
            }
            case Dry:
            {
                mRevModel->setdry(value);
                break;
            }
            case Width:
            {
                mRevModel->setwidth(value);
                break;
            }
            }
        }

        float Reverb::GetParameter(EReverbParameters parameter)
        {
            float ret;
            switch (parameter)
            {
            case PreDelay:
            {
                ret = (float)mDelayLine->GetDelayMs();
                break;
            }
            case Mode:
            {
                ret = mRevModel->getmode();
                break;
            }
            case RoomSize:
            {
                ret = mRevModel->getroomsize();
                break;
            }
            case Damp:
            {
                ret = mRevModel->getdamp();
                break;
            }
            case Wet:
            {
                ret = mRevModel->getwet();
                break;
            }
            case Dry:
            {
                ret = mRevModel->getdry();
                break;
            }
            case Width:
            {
                ret = mRevModel->getwidth();
                break;
            }
            }
            return ret;
        }

        const char* Reverb::GetParameterName(EReverbParameters parameter)
        {
            switch (parameter)
            {
            case PreDelay:
            {
                return "Pre-delay";
                break;
            }
            case Mode:
            {
                return "Mode";
                break;
            }
            case RoomSize:
            {
                return "Room size";
                break;
            }
            case Damp:
            {
                return "Damping";
                break;
            }
            case Wet:
            {
                return "Wet level";
                break;
            }
            case Dry:
            {
                return "Dry level";
                break;
            }
            case Width:
            {
                return "Width";
                break;
            }
            }
        }

        const std::string& Reverb::GetParameterDisplay(EReverbParameters parameter)
        {
            switch (parameter)
            {
            case PreDelay:
            {
                std::to_string(mDelayLine->GetDelayMs()).c_str();
                break;
            }
            case Mode:
            {
                if (mRevModel->getmode() >= freezemode)
                {
                    return "Freeze";
                }
                else
                {
                    return "Normal";
                }
                break;
            }
            case RoomSize:
            {
                return std::to_string(mRevModel->getroomsize() * scaleroom + offsetroom);
                break;
            }
            case Damp:
            {
                return std::to_string((long)(mRevModel->getdamp() * 100));
                break;
            }
            case Wet:
            {
                return std::to_string(mRevModel->getwet() * scalewet);
                break;
            }
            case Dry:
            {
                return std::to_string(mRevModel->getdry() * scaledry);
                break;
            }
            case Width:
            {
                return std::to_string((long)(mRevModel->getwidth() * 100));
                break;
            }
            }
        }

        const char* Reverb::GetParameterLabel(EReverbParameters parameter)
        {
            switch (parameter)
            {
            case PreDelay:
            {
                return "ms";
                break;
            }
            case Mode:
            {
                return "mode";
                break;
            }
            case RoomSize:
            {
                return "size";
                break;
            }
            case Damp:
            case Width:
            {
                return "%";
                break;
            }
            case Wet:
            case Dry:
            {
                return "dB";
                break;
            }
            }
        }
    }
}