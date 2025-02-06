#include "nrpch.h"
#include "FrameBuffer.h"

#include "NotRed/Platform/Vulkan/VKFrameBuffer.h"

#include "NotRed/Renderer/RendererAPI.h"

namespace NR
{
	Ref<FrameBuffer> FrameBuffer::Create(const FrameBufferSpecification& spec)
	{
		Ref<FrameBuffer> result = nullptr;

		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:		return nullptr;			
		case RendererAPIType::Vulkan:	result = Ref<VKFrameBuffer>::Create(spec); break;
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

	void FrameBufferPool::Add(const Ref<FrameBuffer>& framebuffer)
	{
		mPool.push_back(framebuffer);
	}
}