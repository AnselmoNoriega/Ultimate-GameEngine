#include "nrpch.h"
#include "GLFramebuffer.h"

#include <glad/glad.h>

namespace NR
{
    GLFramebuffer::GLFramebuffer(const FramebufferStruct& spec)
        :mSpecification(spec)
    {
        Invalidate();
    }

    GLFramebuffer::~GLFramebuffer()
    {
    }

    const FramebufferStruct& GLFramebuffer::GetSpecification() const
    {
        return mSpecification;
    }

    void GLFramebuffer::Invalidate()
    {
        glCreateFramebuffers(1, &mID);
        glBindFramebuffer(GL_FRAMEBUFFER, mID);

        glCreateTextures(GL_TEXTURE_2D, 1, &mTextureID);
        glBindTexture(GL_TEXTURE_2D, mTextureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, mSpecification.Width, mSpecification.Height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTextureID, 0);

        unsigned int depthAttachment;
        glCreateTextures(GL_TEXTURE_2D, 1, &depthAttachment);
        glBindTexture(GL_TEXTURE_2D, depthAttachment);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, mSpecification.Width, mSpecification.Height);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthAttachment, 0);

        NR_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Incomplete process!");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GLFramebuffer::Bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mID);
    }

    void GLFramebuffer::Unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}