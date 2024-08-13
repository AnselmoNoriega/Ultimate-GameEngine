#include "nrpch.h"
#include "RenderCommandQueue.h"

#define NR_RENDER_TRACE(...) NR_CORE_TRACE(__VA_ARGS__)

namespace NR
{
	RenderCommandQueue::RenderCommandQueue()
	{
		mCommandBuffer = new unsigned char[10 * 1024 * 1024]; // 10mb buffer
		mCommandBufferPtr = mCommandBuffer;
		memset(mCommandBuffer, 0, 10 * 1024 * 1024);
	}

	RenderCommandQueue::~RenderCommandQueue()
	{
		delete[] mCommandBuffer;
	}

	void RenderCommandQueue::Submit(const RenderCommand& command)
	{
		auto ptr = mCommandBuffer;

		memcpy(mCommandBuffer, &command, sizeof(RenderCommand));
		mCommandBufferPtr += sizeof(RenderCommand);
		mRenderCommandCount++;
	}

	void RenderCommandQueue::SubmitCommand(RenderCommandFn fn, void* params, uint32_t size)
	{
		byte*& buffer = mCommandBufferPtr;
		memcpy(buffer, &fn, sizeof(RenderCommandFn));
		buffer += sizeof(RenderCommandFn);
		memcpy(buffer, params, size);
		buffer += size;

		auto totalSize = sizeof(RenderCommandFn) + size;
		auto padding = totalSize % 16; // 16-byte alignment
		buffer += padding;

		mRenderCommandCount++;
	}

	void RenderCommandQueue::Execute()
	{
		NR_RENDER_TRACE("RenderCommandQueue::Execute -- {0} commands, {1} bytes", mRenderCommandCount, (mCommandBufferPtr - mCommandBuffer));

		byte* buffer = mCommandBuffer;

		for (int i = 0; i < mRenderCommandCount; i++)
		{
			RenderCommandFn fn = *(RenderCommandFn*)buffer;
			buffer += sizeof(RenderCommandFn);
			buffer += (*fn)(buffer);

			auto padding = (int)buffer % 16;
			buffer += padding;
		}

		mCommandBufferPtr = mCommandBuffer;
		mRenderCommandCount = 0;
	}
}