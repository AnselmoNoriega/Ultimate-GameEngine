#pragma once

#include <vector>
#include <cassert>
#include <algorithm>
#include <cmath>

#include "NotRed/Audio/SourceManager.h"

namespace NR::Audio
{
	namespace DSP
	{
		class DelayLine
		{
		public:
			explicit DelayLine(int maximumDelayInSamples = 0)
			{
				assert(maximumDelayInSamples >= 0);
				mTotalSize = std::max(4, maximumDelayInSamples + 1);
				mSampleRate = 44100.0;
			}

			void SetDelay(float newDelayInSamples)
			{
				auto upperLimit = (float)(mTotalSize - 1);
				if (newDelayInSamples <= 0)
				{
					newDelayInSamples = 1;
				}
				assert(newDelayInSamples > 0 && newDelayInSamples < upperLimit);

				mDelay = std::clamp(newDelayInSamples, (float)0, upperLimit);
				mDelayInt = static_cast<int> (std::floor(mDelay));
			}

			void SetDelayMs(uint32_t milliseconds)
			{
				SetDelay((double)milliseconds / 1000.0 * mSampleRate);
			}

			void SetConfig(uint32_t numChannels, double sampleRate)
			{
				assert(numChannels > 0);
				mBufferData.resize(numChannels);

				for (auto& ch : mBufferData)
				{
					ch.resize(mTotalSize);
					std::fill(ch.begin(), ch.end(), 0.0f);
				}

				mWritePos.resize(numChannels);
				mReadPos.resize(numChannels);
				mSampleRate = sampleRate;
				Reset();
			}

			void Reset()
			{
				for (auto vec : { &mWritePos, &mReadPos })
				{
					std::fill(vec->begin(), vec->end(), 0);
				}

				for (auto& ch : mBufferData)
				{
					std::fill(ch.begin(), ch.end(), 0.0f);
				}
			}

			void PushSample(int channel, float sample)
			{
				mBufferData.at(channel).at(mWritePos[(size_t)channel]) = sample;
				mWritePos[(size_t)channel] = (mWritePos[(size_t)channel] + mTotalSize - 1) % mTotalSize;
			}

			float PopSample(int channel, float delayInSamples = -1, bool updateReadPointer = true)
			{
				if (delayInSamples >= 0)
				{
					SetDelay(delayInSamples);
				}

				float result = [&, channel]() {
					auto index = (mReadPos[(size_t)channel] + mDelayInt) % mTotalSize;
					return mBufferData.at(channel).at(index);
					}();

				if (updateReadPointer)
				{
					mReadPos[(size_t)channel] = (mReadPos[(size_t)channel] + mTotalSize - 1) % mTotalSize;
				}

				return result;
			}

			float GetDelay() const { return mDelay; }
			uint32_t GetDelayMs() const { return mDelay / mSampleRate * 1000.0; }
			double GetSampleRate() { return mSampleRate; }

		private:
			double mSampleRate;

			std::vector<std::vector<float, SourceManager::Allocator<float>>> mBufferData;
			std::vector<int> mWritePos, mReadPos;

			float mDelay = 0.0;

			int mDelayInt = 0, mTotalSize = 4;
		};
	}
}