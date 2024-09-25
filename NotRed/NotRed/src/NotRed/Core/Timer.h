#pragma once

#include <chrono>

namespace NR
{
	class Timer
	{
	public:
		Timer()
		{
			Reset();
		}

		void Reset()
		{
			mStart = std::chrono::high_resolution_clock::now();
		}

		float Elapsed()
		{
			return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - mStart).count() * 0.001f * 0.001f * 0.001f;
		}

		float ElapsedMillis()
		{
			return Elapsed() * 1000.0f;
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> mStart;
	};

	class PerformanceProfiler
	{
	public:
		void SetPerFrameTiming(const char* name, float time)
		{
			if (mPerFrameData.find(name) == mPerFrameData.end())
			{
				mPerFrameData[name] = 0.0f;
			}
			mPerFrameData[name] += time;
		}

		void Clear() { mPerFrameData.clear(); }

		const std::unordered_map<const char*, float>& GetPerFrameData() const { return mPerFrameData; }

	private:
		std::unordered_map<const char*, float> mPerFrameData;
	};

	class ScopePerformanceTimer
	{
	public:
		ScopePerformanceTimer(const char* name, PerformanceProfiler* profiler)
			: mName(name), mProfiler(profiler) {}

		~ScopePerformanceTimer()
		{
			float time = mTimer.ElapsedMillis();
			mProfiler->SetPerFrameTiming(mName, time);
		}

	private:
		const char* mName;
		PerformanceProfiler* mProfiler;
		Timer mTimer;
	};

#define NR_SCOPE_PERF(name)\
	ScopePerformanceTimer timer__LINE__(name, Application::Get().GetPerformanceProfiler());
}