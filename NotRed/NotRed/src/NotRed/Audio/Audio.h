#pragma once

#include <thread>
#include <atomic>
#include <queue>

#include "NotRed/Core/Timer.h"
#include "NotRed/Core/TimeFrame.h"

namespace NR::Audio
{
#define PCM_FRAME_CHUNK_SIZE 1024
#define STOPPING_FADE_MS 28

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
        friend class AudioEngine;
    };

    static inline bool IsAudioThread()
    {
        return std::this_thread::get_id() == AudioThread::GetThreadID();
    }
}