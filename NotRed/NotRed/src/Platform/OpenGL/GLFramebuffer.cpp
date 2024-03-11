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
        glDeleteFramebuffers(1, &mID);
        glDeleteTextures(1, &mTextureID);
        glDeleteRenderbuffers(1, &mDepthID);
    }

    const FramebufferStruct& GLFramebuffer::GetSpecification() const
    {
        return mSpecification;
    }

    void GLFramebuffer::Invalidate()
    {
        if (mID)
        {
            glDeleteFramebuffers(1, &mID);
            glDeleteTextures(1, &mTextureID);
            glDeleteRenderbuffers(1, &mDepthID);
        }

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

        glGenRenderbuffers(1, &mDepthID);
        glBindRenderbuffer(GL_RENDERBUFFER, mDepthID);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, mSpecification.Width, mSpecification.Height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mDepthID);

        NR_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Incomplete process!");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GLFramebuffer::Resize(uint32_t width, uint32_t height)
    {
        if (width == 0 || width >= 8100 || height == 0 || height >= 8100)
        {
            NR_CORE_WARN("Attempted to rezize with unusable values: {0}, {1}", width, height);
            return;
        }

        mSpecification.Width = width;
        mSpecification.Height = height;

        Invalidate();
    }

    void GLFramebuffer::Bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mID);
        glViewport(0, 0, mSpecification.Width, mSpecification.Height);
    }

    void GLFramebuffer::Unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}