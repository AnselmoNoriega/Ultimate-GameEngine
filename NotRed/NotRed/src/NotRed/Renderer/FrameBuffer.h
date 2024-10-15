#pragma once

#include <glm/glm.hpp>

#include "Image.h"
#include "NotRed/Renderer/RendererTypes.h"

namespace NR
{
	class FrameBuffer;

	enum class FrameBufferTextureFormat
	{
		None,
		RGBA8,
		RGBA16F,
		RGBA32F,
		RG32F,

		DEPTH32F,
		DEPTH24STENCIL8,
		Depth = DEPTH24STENCIL8
	};

	struct FrameBufferTextureSpecification
	{
		FrameBufferTextureSpecification() = default;
		FrameBufferTextureSpecification(ImageFormat format) 
			: Format(format) {}

		ImageFormat Format;
	};

	struct FrameBufferAttachmentSpecification
	{
		FrameBufferAttachmentSpecification() = default;
		FrameBufferAttachmentSpecification(const std::initializer_list<FrameBufferTextureSpecification>& attachments)
			: Attachments(attachments) {}

		std::vector<FrameBufferTextureSpecification> Attachments;
	};

	struct FrameBufferSpecification
	{
		float Scale = 1.0f;
		uint32_t Width = 0;
		uint32_t Height = 0;
		glm::vec4 ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		bool ClearOnLoad = true;
		FrameBufferAttachmentSpecification Attachments;
		uint32_t Samples = 1;

		bool Resizable = true;

		bool SwapChainTarget = false;

		Ref<Image2D> ExistingImage;
		uint32_t ExistingImageLayer;

		Ref<FrameBuffer> ExistingFrameBuffer;

		std::string DebugName;
	};

	class FrameBuffer : public RefCounted
	{
	public:
		static Ref<FrameBuffer> Create(const FrameBufferSpecification& spec);

		virtual ~FrameBuffer() = default;
		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate = false) = 0;
		virtual void AddResizeCallback(const std::function<void(Ref<FrameBuffer>)>& func) = 0;

		virtual void BindTexture(uint32_t attachmentIndex = 0, uint32_t slot = 0) const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual RendererID GetRendererID() const = 0;
		virtual Ref<Image2D> GetImage(uint32_t attachmentIndex = 0) const = 0;
		virtual Ref<Image2D> GetDepthImage() const = 0;

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