#include "nrpch.h"
#include "GLFrameBuffer.h"

#include "NotRed/Renderer/Renderer.h"
#include <glad/glad.h>

namespace NR
{
    GLFrameBuffer::GLFrameBuffer(const FrameBufferSpecification& spec)
        : mSpecification(spec)
    {
        Resize(spec.Width, spec.Height);
    }

    GLFrameBuffer::~GLFrameBuffer()
    {
        Renderer::Submit([this]() {
            glDeleteFramebuffers(1, &mID);
            });
    }

    void GLFrameBuffer::Resize(uint32_t width, uint32_t height)
    {
        if (mSpecification.Width == width && mSpecification.Height == height)
        {
            return;
        }

        mSpecification.Width = width;
        mSpecification.Height = height;

        Renderer::Submit([this]() {
            if (mID)
            {
                glDeleteFramebuffers(1, &mID);
                glDeleteTextures(1, &mColorAttachment);
                glDeleteTextures(1, &mDepthAttachment);
            }

            glGenFramebuffers(1, &mID);
            glBindFramebuffer(GL_FRAMEBUFFER, mID);

            bool multisample = mSpecification.Samples > 1;
            if (multisample)
            {
                glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &mColorAttachment);
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, mColorAttachment);

                if (mSpecification.Format == FrameBufferFormat::RGBA16F)
                {
                    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, mSpecification.Samples, GL_RGBA16F, mSpecification.Width, mSpecification.Height, GL_FALSE);
                }
                else if (mSpecification.Format == FrameBufferFormat::RGBA8)
                {
                    // glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 8, GL_RGBA8, mSpecification.Width, mSpecification.Height, GL_TRUE);
                    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, mSpecification.Samples, GL_RGBA8, mSpecification.Width, mSpecification.Height, GL_FALSE);
                }
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
            }
            else
            {
                glCreateTextures(GL_TEXTURE_2D, 1, &mColorAttachment);
                glBindTexture(GL_TEXTURE_2D, mColorAttachment);

                if (mSpecification.Format == FrameBufferFormat::RGBA16F)
                {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, mSpecification.Width, mSpecification.Height, 0, GL_RGBA, GL_FLOAT, nullptr);
                }
                else if (mSpecification.Format == FrameBufferFormat::RGBA8)
                {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mSpecification.Width, mSpecification.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                }
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorAttachment, 0);
            }

            if (multisample)
            {
                glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &mDepthAttachment);
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, mDepthAttachment);
                glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, mSpecification.Samples, GL_DEPTH24_STENCIL8, mSpecification.Width, mSpecification.Height, GL_FALSE);
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
            }
            else
            {
                glCreateTextures(GL_TEXTURE_2D, 1, &mDepthAttachment);
                glBindTexture(GL_TEXTURE_2D, mDepthAttachment);
                glTexImage2D(
                    GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, mSpecification.Width, mSpecification.Height, 0,
                    GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL
                );
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, mDepthAttachment, 0);
            }

            if (multisample)
            {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, mColorAttachment, 0);
            }
            else
            {
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mColorAttachment, 0);
            }

            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, mDepthAttachment, 0);


            NR_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            });
    }

    void GLFrameBuffer::Bind() const
    {
        Renderer::Submit([=]() {
            glBindFramebuffer(GL_FRAMEBUFFER, mID);
            glViewport(0, 0, mSpecification.Width, mSpecification.Height);
            });
    }

    void GLFrameBuffer::Unbind() const
    {
        Renderer::Submit([=]() {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            });
    }

    void GLFrameBuffer::BindTexture(uint32_t slot) const
    {
        Renderer::Submit([=]() {
            glBindTextureUnit(slot, mColorAttachment);
            });
    }
}