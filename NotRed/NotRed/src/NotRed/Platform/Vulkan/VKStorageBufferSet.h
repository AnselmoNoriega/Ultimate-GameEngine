#pragma once

#include <map>

#include "NotRed/Renderer/StorageBufferSet.h"

#include "NotRed/Core/Assert.h"

namespace NR
{
	class VulkanStorageBufferSet : public StorageBufferSet
	{
	public:
		explicit VulkanStorageBufferSet(uint32_t frames)
			: mFrames(frames) {}

		~VulkanStorageBufferSet() override = default;

		void Create(uint32_t size, uint32_t binding) override
		{
			for (uint32_t frame = 0; frame < mFrames; frame++)
			{
				const Ref<StorageBuffer> storageBuffer = StorageBuffer::Create(size, binding);
				Set(storageBuffer, 0, frame);
			}
		}

		void Resize(const uint32_t binding, const uint32_t set, const uint32_t newSize) override
		{
			for (uint32_t frame = 0; frame < mFrames; frame++)
			{
				mStorageBuffers.at(frame).at(set).at(binding)->Resize(newSize);
			}
		}

		Ref<StorageBuffer> Get(uint32_t binding, uint32_t set = 0, uint32_t frame = 0) override
		{
			NR_CORE_ASSERT(mStorageBuffers.find(frame) != mStorageBuffers.end());
			NR_CORE_ASSERT(mStorageBuffers.at(frame).find(set) != mStorageBuffers.at(frame).end());
			NR_CORE_ASSERT(mStorageBuffers.at(frame).at(set).find(binding) != mStorageBuffers.at(frame).at(set).end());
			return mStorageBuffers.at(frame).at(set).at(binding);
		}

		void Set(Ref<StorageBuffer> storageBuffer, uint32_t set = 0, uint32_t frame = 0) override
		{
			mStorageBuffers[frame][set][storageBuffer->GetBinding()] = storageBuffer;
		}
		
	private:
		uint32_t mFrames;
		std::map<uint32_t, std::map<uint32_t, std::map<uint32_t, Ref<StorageBuffer>>>> mStorageBuffers; // frame->set->binding
	};
}