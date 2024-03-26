#pragma once

namespace NR
{
    enum class FramebufferTextureFormat
    {
        None,
        RGBA8,
        RED_INTEGER,
        DEPTH24STENCIL8,
        Depth = DEPTH24STENCIL8
    };

    struct FramebufferTextureSpecification
    {
        FramebufferTextureSpecification() = default;
        FramebufferTextureSpecification(FramebufferTextureFormat format)
            :TextureFormat(format) {}

        FramebufferTextureFormat TextureFormat = FramebufferTextureFormat::None;
    };

    struct FramebufferAttachmentSpecification
    {
        FramebufferAttachmentSpecification() = default;
        FramebufferAttachmentSpecification(std::initializer_list<FramebufferTextureSpecification> attachments)
            : AttachmentsVector(attachments) {}

        std::vector<FramebufferTextureSpecification> AttachmentsVector;
    };

    struct FramebufferStruct
    {
        uint32_t Width, Height;
        uint32_t Samples = 1;

        FramebufferAttachmentSpecification Attachments;

        bool SwapChainTarget = false;
    };

    class Framebuffer
    {
    public:
        virtual ~Framebuffer() = default;

        virtual void Bind() = 0;
        virtual void Unbind() = 0;

        virtual void Resize(uint32_t width, uint32_t height) = 0;
        virtual int GetPixel(uint32_t attachmentIndex, int x, int y) = 0;

        virtual void ClearAttachment(uint32_t attachmentIndex, int value) = 0;

        virtual const FramebufferStruct& GetSpecification() const = 0;
        virtual uint32_t GetTextureRendererID(uint32_t index = 0) const = 0;

        static Ref<Framebuffer> Create(const FramebufferStruct& spec);
    };
}
