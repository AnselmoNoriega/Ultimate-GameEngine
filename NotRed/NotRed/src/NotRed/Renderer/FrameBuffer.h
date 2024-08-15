#pragma once

#include "NotRed/Renderer/RendererAPI.h"

namespace NR
{
	enum class FrameBufferFormat
	{
		None,
		RGBA8,
		RGBA16F
	};

	class FrameBuffer
	{
	public:
		static FrameBuffer* Create(uint32_t width, uint32_t height, FrameBufferFormat format);

		virtual ~FrameBuffer() {}
		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual void BindTexture(uint32_t slot = 0) const = 0;

		virtual RendererID GetRendererID() const = 0;
		virtual RendererID GetColorAttachmentRendererID() const = 0;
		virtual RendererID GetDepthAttachmentRendererID() const = 0;
	};

	class FrameBufferPool final
	{
	public:
		FrameBufferPool(uint32_t maxFBs = 32);
		~FrameBufferPool();

		std::weak_ptr<FrameBuffer> AllocateBuffer();
		void Add(FrameBuffer* FrameBuffer);

		const std::vector<FrameBuffer*>& GetAll() const { return mPool; }

		inline static FrameBufferPool* GetGlobal() { return sInstance; }

	private:
		std::vector<FrameBuffer*> mPool;

		static FrameBufferPool* sInstance;
	};

}