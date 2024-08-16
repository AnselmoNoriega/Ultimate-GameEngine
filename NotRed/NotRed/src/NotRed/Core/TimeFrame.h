#pragma once

namespace NR
{
	class TimeFrame
	{
	public:
		TimeFrame() = default;
		TimeFrame(float time);

		inline float GetSeconds() const { return mTime; }
		inline float GetMilliseconds() const { return mTime * 1000.0f; }

		operator float() { return mTime; }

	private:
		float mTime = 0.0f;
	};
}