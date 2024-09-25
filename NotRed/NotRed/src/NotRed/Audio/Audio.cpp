#include "nrpch.h"
#include "Audio.h"

namespace NR::Audio
{
    bool AudioThread::Start()
    {
        if (sThreadActive)
        {
            return;
        }

        sThreadActive = true;
        sAudioThread = new std::thread([]{

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
        std::scoped_lock lock(sAudioThreadJobsLock);
        sAudioThreadJobs.emplace(std::move(funcCb));
    }

    void AudioThread::Update()
    {
        sTimer.Reset();

        // Handle AudioThread Jobs
        std::scoped_lock lock(sAudioThreadJobsLock);
        if (!sAudioThreadJobs.empty())
        {
            for (int i = sAudioThreadJobs.size() - 1; i >= 0; --i)
            {
                auto job = sAudioThreadJobs.front();
                job->Execute();

                sAudioThreadJobs.pop();
                delete job;
            }
        }
        NR_CORE_ASSERT(mUpdateCallback != nullptr, "Update Function is not bound!");

        mUpdateCallback(sTimeFrame);
        sTimeFrame = sTimer.Elapsed();
        sLastFrameTime = sTimeFrame.GetMilliseconds();
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