#pragma once

#include "NotRed/Core/Ref.h"

namespace NR
{
	class StorageBuffer : public RefCounted
	{
	public:
		virtual ~StorageBuffer() = default;

		static Ref<StorageBuffer> Create(uint32_t size, uint32_t binding);

		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;
		virtual void RT_SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;
		virtual void Resize(uint32_t newSize) = 0;

		virtual uint32_t GetBinding() const = 0;
	};
}