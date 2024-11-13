#pragma once

#include "NotRed/Renderer/UniformBufferSet.h"

namespace NR
{
	class GLUniformBufferSet : public UniformBufferSet
	{
	public:
		GLUniformBufferSet(uint32_t frames) {};
		~GLUniformBufferSet() = default;

		void Create(uint32_t size, uint32_t binding) override {}

		Ref<UniformBuffer> Get(uint32_t binding, uint32_t set = 0, uint32_t frame = 0) override { return nullptr; }
		void Set(Ref<UniformBuffer> uniformBuffer, uint32_t set = 0, uint32_t frame = 0) override { }
	};
}