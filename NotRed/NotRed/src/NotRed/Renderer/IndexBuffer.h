#pragma once

#include "NotRed/Renderer/Renderer.h"

namespace NR
{
	class NOT_RED_API IndexBuffer
	{
	public:
		virtual ~IndexBuffer() {}

		virtual void SetData(void* buffer, uint32_t size, uint32_t offset = 0) = 0;
		virtual void Bind() const = 0;

		virtual uint32_t GetSize() const = 0;
		virtual uint32_t GetCount() const = 0;
		virtual RendererID GetRendererID() const = 0;

		static IndexBuffer* Create(uint32_t size = 0);
	};
}