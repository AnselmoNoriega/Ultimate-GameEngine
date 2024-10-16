#include "nrpch.h"
#include "Audio.h"

#include <chrono>

#include "NotRed/Debug/Profiler.h"

using namespace std::chrono_literals;

namespace NR::Audio
{
    static std::queue<AudioFunctionCallback*> sAudioThreadJobsLocal;

    bool AudioThread::Start()
    {
        if (sThreadActive)
        {
            return false;
        }

        sThreadActive = true;
        sAudioThread = new std::thread([]
            {
                NR_PROFILE_THREAD("AudioThread");

#if defined(NR_PLATFORM_WINDOWS)
                HRESULT r;
                r = SetThreadDescription(GetCurrentThread(), L"NotRed Audio Thread");
#endif
                NR_CORE_INFO("Spinning up Audio Thread.");
                while (sThreadActive)
                {
                    Update();
                }
                NR_CORE_INFO("Audio Thread stopped.");
            });

        sAudioThreadID = sAudioThread->get_id();
        sAudioThread->detach();

        return true;
    }

    bool AudioThread::Stop()
    {
        if (!sThreadActive)
        {
            return false;
        }

        sThreadActive = false;
        return true;
    }

    bool AudioThread::IsRunning()
    {
        return sThreadActive;
    }

    bool AudioThread::IsAudioThread()
    {
        return std::this_thread::get_id() == sAudioThreadID;
    }

    std::thread::id AudioThread::GetThreadID()
    {
        return sAudioThreadID;
    }

    void AudioThread::AddTask(AudioFunctionCallback*&& funcCb)
    {
        NR_PROFILE_FUNC();

        std::scoped_lock lock(sAudioThreadJobsLock);
        sAudioThreadJobs.emplace(std::move(funcCb));
    }

    void AudioThread::Update()
    {
        NR_PROFILE_FUNC();

        sTimer.Reset();

        // Handle AudioThread Jobs
        {
            NR_PROFILE_FUNC("AudioThread::Update - Execution");

            auto& jobs = sAudioThreadJobsLocal;
            {
                std::scoped_lock lock(sAudioThreadJobsLock);
                sAudioThreadJobsLocal = sAudioThreadJobs;
                sAudioThreadJobs = std::queue<AudioFunctionCallback*>();
            }

            if (!jobs.empty())
            {
                for (int i = (int)jobs.size() - 1; i >= 0; --i)
                {
                    auto job = jobs.front();

                    job->Execute();

                    jobs.pop();
                    delete job;
                }
            }
        }
        NR_CORE_ASSERT(mUpdateCallback != nullptr, "Update Function is not bound!");

        mUpdateCallback(sTimeFrame);
        sTimeFrame = sTimer.Elapsed();
        sLastFrameTime = sTimeFrame.GetMilliseconds();   
        
        std::this_thread::sleep_for(1ms);
    }

    std::thread* AudioThread::sAudioThread = nullptr;
    std::atomic<bool> AudioThread::sThreadActive = false;
    std::atomic<std::thread::id> AudioThread::sAudioThreadID = std::thread::id();

    std::queue<AudioFunctionCallback*> AudioThread::sAudioThreadJobs;
    std::mutex AudioThread::sAudioThreadJobsLock;
    std::function<void(TimeFrame)> AudioThread::mUpdateCallback = nullptr;

    Timer AudioThread::sTimer;
    std::atomic<float> AudioThread::sLastFrameTime = 0.0f;
    TimeFrame AudioThread::sTimeFrame = 0.0f;
}