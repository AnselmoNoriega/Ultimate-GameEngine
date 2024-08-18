#include "nrpch.h"
#include "FrameBuffer.h"

#include "NotRed/Platform/OpenGL/GLFrameBuffer.h"

namespace NR
{
	Ref<FrameBuffer> FrameBuffer::Create(const FrameBufferSpecification& spec)
	{
		Ref<FrameBuffer> result = nullptr;

		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:		return nullptr;
		case RendererAPIType::OpenGL:	result = std::make_shared<GLFrameBuffer>(spec);
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

	void FrameBufferPool::Add(std::weak_ptr<FrameBuffer> framebuffer)
	{
		mPool.push_back(framebuffer);
	}

}