#pragma once

#include "NotREd/Core/Ref.h"

namespace NR
{
	class UniformBuffer : public RefCounted
	{
	public:
		virtual ~UniformBuffer() = default;
		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;
		virtual void RT_SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;

		virtual uint32_t GetBinding() const = 0;

		static Ref<UniformBuffer> Create(uint32_t size, uint32_t binding);
	};
}