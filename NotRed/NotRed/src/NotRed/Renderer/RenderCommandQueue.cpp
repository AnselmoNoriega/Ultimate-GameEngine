#include "nrpch.h"
#include "RenderCommandQueue.h"

#define NR_RENDER_TRACE(...) NR_CORE_TRACE(__VA_ARGS__)

namespace NR
{
	RenderCommandQueue::RenderCommandQueue()
	{
		mCommandBuffer = new uint8_t[10 * 1024 * 1024];
		mCommandBufferPtr = mCommandBuffer;
		memset(mCommandBuffer, 0, 10 * 1024 * 1024);
	}

	RenderCommandQueue::~RenderCommandQueue()
	{
		delete[] mCommandBuffer;
	}

	void* RenderCommandQueue::Allocate(RenderCommandFn fn, uint32_t size)
	{
		*(RenderCommandFn*)mCommandBufferPtr = fn;
		mCommandBufferPtr += sizeof(RenderCommandFn);

		*(uint32_t*)mCommandBufferPtr = size;
		mCommandBufferPtr += sizeof(uint32_t);

		void* memory = mCommandBufferPtr;
		mCommandBufferPtr += size;

		++mCommandCount;
		return memory;
	}

	void RenderCommandQueue::Execute()
	{
		byte* buffer = mCommandBuffer;

		for (uint32_t i = 0; i < mCommandCount; ++i)
		{
			RenderCommandFn function = *(RenderCommandFn*)buffer;
			buffer += sizeof(RenderCommandFn);

			uint32_t size = *(uint32_t*)buffer;
			buffer += sizeof(uint32_t);
			function(buffer);
			buffer += size;
		}

		mCommandBufferPtr = mCommandBuffer;
		mCommandCount = 0;
	}
}