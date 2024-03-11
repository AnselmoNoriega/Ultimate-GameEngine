#pragma once

#include "NotRed/Renderer/Framebuffer.h"

namespace NR
{
    class GLFramebuffer : public Framebuffer
    {
    public:
        GLFramebuffer(const FramebufferStruct& spec);
        ~GLFramebuffer() override;

        void Bind() override;
        void Unbind() override;

        void Resize(uint32_t width, uint32_t height) override;

        const FramebufferStruct& GetSpecification() const override;
        uint32_t GetTextureRendererID() const override { return mTextureID; }


    private:
        void Invalidate();

    private:
        uint32_t mID = 0, 
                 mTextureID = 0, 
                 mDepthID = 0;

        FramebufferStruct mSpecification;
    };
}