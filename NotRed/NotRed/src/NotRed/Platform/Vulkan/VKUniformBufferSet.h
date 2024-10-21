#pragma once

#include <map>

#include "NotRed/Renderer/UniformBufferSet.h"

namespace NR
{
	class VKUniformBufferSet : public UniformBufferSet
	{
	public:
		VKUniformBufferSet(uint32_t frames)
			: mFrames(frames) {}

		~VKUniformBufferSet() override = default;

		void Create(uint32_t size, uint32_t binding) override
		{
			for (uint32_t frame = 0; frame < mFrames; frame++)
			{
				Ref<UniformBuffer> uniformBuffer = UniformBuffer::Create(size, binding);
				Set(uniformBuffer, 0, frame);
			}
		}

		Ref<UniformBuffer> Get(uint32_t binding, uint32_t set = 0, uint32_t frame = 0) override
		{
			NR_CORE_ASSERT(mUniformBuffers.find(frame) != mUniformBuffers.end());
			NR_CORE_ASSERT(mUniformBuffers.at(frame).find(set) != mUniformBuffers.at(frame).end());
			NR_CORE_ASSERT(mUniformBuffers.at(frame).at(set).find(binding) != mUniformBuffers.at(frame).at(set).end());
			return mUniformBuffers.at(frame).at(set).at(binding);
		}

		void Set(Ref<UniformBuffer> uniformBuffer, uint32_t set = 0, uint32_t frame = 0) override
		{
			mUniformBuffers[frame][set][uniformBuffer->GetBinding()] = uniformBuffer;
		}

	private:
		uint32_t mFrames;
		std::map<uint32_t, std::map<uint32_t, std::map<uint32_t, Ref<UniformBuffer>>>> mUniformBuffers; // frame->set->binding
	};
}