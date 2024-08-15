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
		unsigned char* mCommandBuffer;
		unsigned char* mCommandBufferPtr;
		uint32_t mCommandCount = 0;
	};
}