#pragma once

#include "nrpch.h"

namespace NR
{
	class RenderCommandQueue
	{
	public:
		typedef void(*RenderCommandFn)(void*);

		RenderCommandQueue();
		~RenderCommandQueue();

		void* Allocate(RenderCommandFn func, uint32_t size);
		void Execute();

	private:
		uint8_t* mCommandBuffer;
		uint8_t* mCommandBufferPtr;
		uint32_t mCommandCount = 0;
	};
}