#pragma once

#include <glm/glm.hpp>

#include "NotRed/Renderer/RendererAPI.h"

namespace NR
{
	enum class FrameBufferFormat
	{
		None,
		RGBA8,
		RGBA16F
	};

	struct FrameBufferSpecification
	{
		uint32_t Width = 1280;
		uint32_t Height = 720;
		glm::vec4 ClearColor;
		FrameBufferFormat Format;
		uint32_t Samples = 1;

		bool SwapChainTarget = false;
	};

	class FrameBuffer : public RefCounted
	{
	public:
		static Ref<FrameBuffer> Create(const FrameBufferSpecification& spec);

		virtual ~FrameBuffer() {}
		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate = false) = 0;

		virtual void BindTexture(uint32_t slot = 0) const = 0;

		virtual RendererID GetRendererID() const = 0;
		virtual RendererID GetColorAttachmentRendererID() const = 0;
		virtual RendererID GetDepthAttachmentRendererID() const = 0;

		virtual const FrameBufferSpecification& GetSpecification() const = 0;
	};

	class FrameBufferPool final
	{
	public:
		FrameBufferPool(uint32_t maxFBs = 32);
		~FrameBufferPool();

		std::weak_ptr<FrameBuffer> AllocateBuffer();
		void Add(const Ref<FrameBuffer>& framebuffer);

		std::vector<Ref<FrameBuffer>>& GetAll() { return mPool; }
		const std::vector<Ref<FrameBuffer>>& GetAll() const { return mPool; }

		inline static FrameBufferPool* GetGlobal() { return sInstance; }

	private:
		std::vector<Ref<FrameBuffer>> mPool;

		static FrameBufferPool* sInstance;
	};

}