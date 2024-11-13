#pragma once

#include "Sound.h"
#include <queue>

namespace NR::Audio
{
    class AudioEngine;

    class SourceManager
    {
    public:
        template<typename T>
        class Allocator : public std::allocator<T>
        {
        private:
            using Base = std::allocator<T>;
            using Pointer = typename std::allocator_traits<Base>::pointer;
            using SizeType = typename std::allocator_traits<Base>::size_type;
            
        public:
            Allocator() = default;
            
            template<typename U>
            Allocator(const Allocator<U>& other)
                : Base(other) {}
            
            template<typename U>
            struct rebind { using other = Allocator<U>; };
            
            Pointer allocate(SizeType n)
            {
                AllocationCallback(n * sizeof(T));
                return Base::allocate(n);
            }
        
            void deallocate(Pointer p, SizeType n)
            {
                DeallocationCallback(n * sizeof(T));
                Base::deallocate(p, n);
            }
        };

    public:
        SourceManager(AudioEngine& audioEngine);
        ~SourceManager();

        void Initialize();
        bool InitializeSource(uint32_t sourceID, const Ref<SoundConfig>& sourceConfig);

        void ReleaseSource(uint32_t sourceID);

        bool GetFreeSourceId(int& sourceIDOut);

        static void SetMasterReverbSendForSource(uint32_t sourceID, float sendLevel);
        static void SetMasterReverbSendForSource(Sound* source, float sendLevel);

    private:
        static void AllocationCallback(uint64_t size);
        static void DeallocationCallback(uint64_t size);

    private:
        AudioEngine& mAudioEngine;
        std::queue<int> mFreeSourcIDs;

    private:
        friend class AudioEngine;
    };
}