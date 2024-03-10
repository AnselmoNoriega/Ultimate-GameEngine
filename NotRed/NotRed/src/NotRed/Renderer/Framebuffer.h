#pragma once

namespace NR
{
    struct FramebufferStruct
    {
        uint32_t Width, Height;
        uint32_t Samples = 1;
        bool SwapChainTarget = false;
    };

    class Framebuffer
    {
    public:
        virtual ~Framebuffer() = default;

        virtual const FramebufferStruct& GetSpecification() const = 0;
        virtual void Bind() = 0;
        virtual void Unbind() = 0;
        virtual uint32_t GetTextureRendererID() const = 0;

        static Ref<Framebuffer> Create(const FramebufferStruct& spec);
    };
}
