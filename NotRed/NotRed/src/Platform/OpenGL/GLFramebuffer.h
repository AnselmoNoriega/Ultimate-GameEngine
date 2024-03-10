#pragma once

#include "NotRed/Renderer/Framebuffer.h"

namespace NR
{
    class GLFramebuffer : public Framebuffer
    {
    public:
        GLFramebuffer(const FramebufferStruct& spec);
        ~GLFramebuffer() override;

        const FramebufferStruct& GetSpecification() const override;
        uint32_t GetTextureRendererID() const override { return mTextureID; }

        void Bind() override;
        void Unbind() override;

    private:
        void Invalidate();

    private:
        uint32_t mID;
        uint32_t mTextureID;

        FramebufferStruct mSpecification;
    };
}