#pragma once

namespace NR
{
    class TimeStep
    {
    public:
        TimeStep() {};

        void UpdateTimeFrame(float time) 
        { 
            mTime = time - mLastFrameTime;
            mLastFrameTime = time;
        }

        operator float() const { return mTime; }

    private:
        float mTime = 0.0f;
        float mLastFrameTime = 0.0f;
    };
}