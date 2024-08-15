#include "nrpch.h"
#include "FrameBuffer.h"

#include "NotRed/Platform/OpenGL/GLFrameBuffer.h"

namespace NR
{
	FrameBuffer* FrameBuffer::Create(uint32_t width, uint32_t height, FrameBufferFormat format)
	{
		NR::FrameBuffer* result = nullptr;

		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:		return nullptr;
		case RendererAPIType::OpenGL:	result = new GLFrameBuffer(width, height, format);
		}
		FrameBufferPool::GetGlobal()->Add(result);
		return result;
	}

	FrameBufferPool* FrameBufferPool::sInstance = new FrameBufferPool;

	FrameBufferPool::FrameBufferPool(uint32_t maxFBs)
	{

	}

	FrameBufferPool::~FrameBufferPool()
	{

	}

	std::weak_ptr<FrameBuffer> FrameBufferPool::AllocateBuffer()
	{
		return std::weak_ptr<FrameBuffer>();
	}

	void FrameBufferPool::Add(FrameBuffer* FrameBuffer)
	{
		mPool.push_back(FrameBuffer);
	}

}