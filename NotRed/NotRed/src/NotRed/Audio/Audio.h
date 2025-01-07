#pragma once

#include <thread>
#include <atomic>
#include <queue>

#include "NotRed/Core/Timer.h"
#include "NotRed/Core/TimeFrame.h"

namespace NR
{
    class AudioEngine;
}

namespace NR::Audio
{
    static constexpr auto PCM_FRAME_CHUNK_SIZE = 1024;
    static constexpr auto STOPPING_FADE_MS = 28;
    static constexpr float SPEED_OF_SOUND = 343.3f;

    using AudioThreadCallbackFunction = std::function<void()>;

    class AudioFunctionCallback
    {
    public:
        AudioFunctionCallback(AudioThreadCallbackFunction f, const char* jobID = "None")
            : mFunc(std::move(f)), mJobID(jobID) {}

        void Execute()
        {
            mFunc();
        }

        const char* GetID() { return mJobID; }

    private:
        AudioThreadCallbackFunction const mFunc;
        const char* mJobID;
    };

    class AudioThread
    {
    public:
        static bool Start();
        static bool Stop();

        static bool IsRunning();
        static bool IsAudioThread();

        static std::thread::id GetThreadID();

    private:
        static void AddTask(AudioFunctionCallback*&& funcCb);

        static void Update();

        static float GetFrameTime() { return sLastFrameTime.load(); }

        template<typename C, void (C::* Function)(TimeFrame)>
        static void BindUpdateFunction(C* instance)
        {
            mUpdateCallback = [instance](TimeFrame dt) {(static_cast<C*>(instance)->*Function)(dt); };
        }

        template<typename FuncT>
        static void BindUpdateFunction(FuncT&& func)
        {
            mUpdateCallback = [func](TimeFrame dt) { func(dt); };
        }

    private:
        static std::thread* sAudioThread;
        static std::atomic<bool> sThreadActive;
        static std::atomic<std::thread::id> sAudioThreadID;

        static std::queue<AudioFunctionCallback*> sAudioThreadJobs;
        static std::mutex sAudioThreadJobsLock;
        static std::function<void(TimeFrame)> mUpdateCallback;

        static Timer sTimer;
        static TimeFrame sTimeFrame;
        static std::atomic<float> sLastFrameTime;
        
    private:
        friend class NR::AudioEngine;
    };

    static inline bool IsAudioThread()
    {
        return std::this_thread::get_id() == AudioThread::GetThreadID();
    }

    struct Transform
    {
        glm::vec3 Position{ 0.0f, 0.0f, 0.0f };
        glm::vec3 Orientation{ 0.0f, 0.0f, -1.0f };
        glm::vec3 Up{ 0.0f, 1.0f, 0.0f };

        bool operator==(const Transform& other) const
        {
            return Position == other.Position && Orientation == other.Orientation && Up == other.Up;
        }

        bool operator!=(const Transform& other) const
        {
            return !(*this == other);
        }
    };

    //==============================================================================

    static inline float Lerp(float a, float b, float t)
    {
        return a + t * (b - a);
    }

    class SampleBufferOperations
    {
    public:
        static inline void ApplyGainRamp(float* data, uint32_t numSamples, uint32_t numChannels, float gainStart, float gainEnd)
        {
            const float delta = (gainEnd - gainStart) / (float)numSamples;
            for (uint32_t i = 0; i < numSamples; ++i)
            {
                for (uint32_t ch = 0; ch < numChannels; ++ch)
                {
                    data[i * numChannels + ch] *= gainStart + delta * i;
                }
            }
        }

        static inline void ApplyGainRampToSingleChannel(float* data, uint32_t numSamples, uint32_t numChannels, uint32_t channel, float gainStart, float gainEnd)
        {
            const float delta = (gainEnd - gainStart) / (float)numSamples;
            for (uint32_t i = 0; i < numSamples; ++i)
            {
                data[i * numChannels + channel] *= gainStart + delta * i;
            }
        }
        static inline void AddAndApplyGainRamp(float* dest, const float* source, uint32_t destChannel, uint32_t sourceChannel,
            uint32_t destNumChannels, uint32_t sourceNumChannels, uint32_t numSamples, float gainStart, float gainEnd)
        {
            if (gainEnd == gainStart)
            {
                for (uint32_t i = 0; i < numSamples; ++i)
                {
                    dest[i * destNumChannels + destChannel] += source[i * sourceNumChannels + sourceChannel] * gainStart;
                }
            }
            else
            {
                const float delta = (gainEnd - gainStart) / (float)numSamples;
                for (uint32_t i = 0; i < numSamples; ++i)
                {
                    dest[i * destNumChannels + destChannel] += source[i * sourceNumChannels + sourceChannel] * gainStart;
                    gainStart += delta;
                }
            }
        }

        static inline void AddAndApplyGain(float* dest, const float* source, uint32_t destChannel, uint32_t sourceChannel,
            uint32_t destNumChannels, uint32_t sourceNumChannels, uint32_t numSamples, float gain)
        {
            for (uint32_t i = 0; i < numSamples; ++i)
            {
                dest[i * destNumChannels + destChannel] += source[i * sourceNumChannels + sourceChannel] * gain;
            }
        }

        static bool ContentMatches(const float* buffer1, const float* buffer2, uint32_t frameCount, uint32_t numChannels)
        {
            for (uint32_t frame = 0; frame < frameCount; ++frame)
            {
                for (uint32_t chan = 0; chan < numChannels; ++chan)
                {
                    const auto pos = frame * numChannels + chan;
                    if (buffer1[pos] != buffer2[pos])
                    {
                        return false;
                    }
                }
            }
            return true;
        }
    };
}