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
        uint32_t GetTextureRendererID(uint32_t index = 0) const override
        {
            NR_CORE_ASSERT(index < mTextureIDs.size(), "Index out of scope");
            return mTextureIDs[index];
        }


    private:
        void Invalidate();

    private:
        uint32_t mID = 0;

        FramebufferStruct mSpecification;

        std::vector<FramebufferTextureSpecification> mTextureAttachmentSpecification;
        FramebufferTextureSpecification mDepthAttachmentSpecification;

        std::vector<uint32_t> mTextureIDs;
        uint32_t mDepthID = 0;
    };
}