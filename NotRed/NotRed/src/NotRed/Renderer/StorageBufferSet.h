#pragma once

#include "StorageBuffer.h"

namespace NR
{
	class StorageBufferSet : public RefCounted
	{
	public:
		virtual ~StorageBufferSet() = default;

		static Ref<StorageBufferSet> Create(uint32_t frames);

		virtual void Create(uint32_t size, uint32_t binding) = 0;

		virtual Ref<StorageBuffer> Get(uint32_t binding, uint32_t set = 0, uint32_t frame = 0) = 0;
		virtual void Set(Ref<StorageBuffer> storageBuffer, uint32_t set = 0, uint32_t frame = 0) = 0;
		virtual void Resize(uint32_t binding, uint32_t set, uint32_t newSize) = 0;
	};
}