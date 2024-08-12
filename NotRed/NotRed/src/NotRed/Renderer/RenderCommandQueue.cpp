#include "nrpch.h"
#include "RenderCommandQueue.h"

#define HZ_RENDER_TRACE(...) HZ_CORE_TRACE(__VA_ARGS__)

namespace NR
{
	RenderCommandQueue::RenderCommandQueue()
	{
		const uint32_t size = 10 * 1024 * 1024;// 10mb buffer
		mCommandBuffer = new uint8_t[size];
		mCommandBufferPtr = mCommandBufferPtr;
		memset(mCommandBufferPtr, 0, size);
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

		for (uint32_t i = 0; i < mCommandCount; i++)
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