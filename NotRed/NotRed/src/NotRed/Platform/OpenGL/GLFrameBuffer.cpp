#include "nrpch.h"
#include "GLFrameBuffer.h"

#include "NotRed/Renderer/Renderer.h"
#include <glad/glad.h>

namespace NR
{
    namespace Utils
    {
        static GLenum TextureTarget(bool multisampled)
        {
            return multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
        }

        static void CreateTextures(bool multisampled, RendererID* outID, uint32_t count)
        {
            glCreateTextures(TextureTarget(multisampled), 1, outID);
        }

        static void BindTexture(bool multisampled, RendererID id)
        {
            glBindTexture(TextureTarget(multisampled), id);
        }

        static GLenum DataType(GLenum format)
        {
            switch (format)
            {
            case GL_RGBA8:               return GL_UNSIGNED_BYTE;
            case GL_RG16F:
            case GL_RG32F:
            case GL_RGBA16F:
            case GL_RGBA32F:             return GL_FLOAT;
            case GL_DEPTH24_STENCIL8:    return GL_UNSIGNED_INT_24_8;
            default:
            {
                NR_CORE_ASSERT(false, "Unknown format!");
                return 0;
            }
            }

        }

        static void AttachColorTexture(RendererID id, int samples, GLenum format, uint32_t width, uint32_t height, int index)
        {
            bool multisampled = samples > 1;
            if (multisampled)
            {
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
            }
            else
            {
                glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGBA, DataType(format), nullptr);

                glTexParameteri(TextureTarget(multisampled), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(TextureTarget(multisampled), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(TextureTarget(multisampled), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(TextureTarget(multisampled), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, TextureTarget(multisampled), id, 0);
        }

        static void AttachDepthTexture(RendererID id, int samples, GLenum format, GLenum attachmentType, uint32_t width, uint32_t height)
        {
            bool multisampled = samples > 1;
            if (multisampled)
            {
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
            }
            else
            {
                glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);

                glTexParameteri(TextureTarget(multisampled), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(TextureTarget(multisampled), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(TextureTarget(multisampled), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(TextureTarget(multisampled), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }

            glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, TextureTarget(multisampled), id, 0);
        }

        static bool IsDepthFormat(FrameBufferTextureFormat format)
        {
            switch (format)
            {
            case FrameBufferTextureFormat::DEPTH24STENCIL8:
            case FrameBufferTextureFormat::DEPTH32F:
                return true;
            }
            return false;
        }

    }

    GLFrameBuffer::GLFrameBuffer(const FrameBufferSpecification& spec)
        : mSpecification(spec), mWidth(spec.Width), mHeight(spec.Height)
    {
        for (auto format : mSpecification.Attachments.Attachments)
        {
            if (!Utils::IsDepthFormat(format.TextureFormat))
            {
                mColorAttachmentFormats.emplace_back(format.TextureFormat);
            }
            else
            {
                mDepthAttachmentFormat = format.TextureFormat;
            }
        }

        Resize(spec.Width, spec.Height, true);
    }

    GLFrameBuffer::~GLFrameBuffer()
    {
        Ref<GLFrameBuffer> instance = this;
        Renderer::Submit([instance]() {
            glDeleteFramebuffers(1, &instance->mID);
            });
    }

    void GLFrameBuffer::Resize(uint32_t width, uint32_t height, bool forceRecreate)
    {
        if (!forceRecreate && mWidth == width && mHeight == height)
        {
            return;
        }

        mWidth = width;
        mHeight = height;

        Ref<GLFrameBuffer> instance = this;
        Renderer::Submit([instance]() mutable
            {
                if (instance->mID)
                {
                    glDeleteFramebuffers(1, &instance->mID);
                    glDeleteTextures(instance->mColorAttachments.size(), instance->mColorAttachments.data());
                    glDeleteTextures(1, &instance->mDepthAttachment);

                    instance->mColorAttachments.clear();
                    instance->mDepthAttachment = 0;
                }

                glGenFramebuffers(1, &instance->mID);
                glBindFramebuffer(GL_FRAMEBUFFER, instance->mID);

                bool multisample = instance->mSpecification.Samples > 1;

                if (instance->mColorAttachmentFormats.size())
                {
                    instance->mColorAttachments.resize(instance->mColorAttachmentFormats.size());
                    Utils::CreateTextures(multisample, instance->mColorAttachments.data(), instance->mColorAttachments.size());

                    for (int i = 0; i < instance->mColorAttachments.size(); i++)
                    {
                        Utils::BindTexture(multisample, instance->mColorAttachments[i]);
                        switch (instance->mColorAttachmentFormats[i])
                        {
                        case FrameBufferTextureFormat::RGBA8:
                            Utils::AttachColorTexture(instance->mColorAttachments[i], instance->mSpecification.Samples, GL_RGBA8, instance->mWidth, instance->mHeight, i);
                            break;
                        case FrameBufferTextureFormat::RGBA16F:
                            Utils::AttachColorTexture(instance->mColorAttachments[i], instance->mSpecification.Samples, GL_RGBA16F, instance->mWidth, instance->mHeight, i);
                            break;
                        case FrameBufferTextureFormat::RGBA32F:
                            Utils::AttachColorTexture(instance->mColorAttachments[i], instance->mSpecification.Samples, GL_RGBA32F, instance->mWidth, instance->mHeight, i);
                            break;
                        case FrameBufferTextureFormat::RG32F:
                            Utils::AttachColorTexture(instance->mColorAttachments[i], instance->mSpecification.Samples, GL_RG32F, instance->mWidth, instance->mHeight, i);
                            break;
                        }

                    }
                }

                if (instance->mDepthAttachmentFormat != FrameBufferTextureFormat::None)
                {
                    Utils::CreateTextures(multisample, &instance->mDepthAttachment, 1);
                    Utils::BindTexture(multisample, instance->mDepthAttachment);
                    switch (instance->mDepthAttachmentFormat)
                    {
                    case FrameBufferTextureFormat::DEPTH24STENCIL8:
                        Utils::AttachDepthTexture(instance->mDepthAttachment, instance->mSpecification.Samples, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT, instance->mWidth, instance->mHeight);
                        break;
                    case FrameBufferTextureFormat::DEPTH32F:
                        Utils::AttachDepthTexture(instance->mDepthAttachment, instance->mSpecification.Samples, GL_DEPTH_COMPONENT32F, GL_DEPTH_ATTACHMENT, instance->mWidth, instance->mHeight);
                        break;
                    }
                }

                if (instance->mColorAttachments.size() > 1)
                {
                    NR_CORE_ASSERT(instance->mColorAttachments.size() <= 4);
                    GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
                    glDrawBuffers(instance->mColorAttachments.size(), buffers);
                }
                else if (instance->mColorAttachments.size() == 0)
                {
                    // Only depth-pass
                    glDrawBuffer(GL_NONE);
                }

#if 0
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
                    else if (mSpecification.Format == FramebufferFormat::RG32F)
                    {
                        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, mSpecification.Width, mSpecification.Height);
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_ColorAttachment, 0);
                        glDrawBuffer(GL_NONE);
                    }
                    else if (mSpecification.Format == FrameBufferFormat::RGBA8)
                    {
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mSpecification.Width, mSpecification.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                    }
                    else if (mSpecification.Format == FramebufferFormat::COMP)
                    {
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, mSpecification.Width, mSpecification.Height, 0, GL_RGBA, GL_FLOAT, nullptr);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                        glCreateTextures(GL_TEXTURE_2D, 1, &m_ColorAttachment2);
                        glBindTexture(GL_TEXTURE_2D, m_ColorAttachment2);
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, mSpecification.Width, mSpecification.Height, 0, GL_RGBA, GL_FLOAT, nullptr);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                    }

                    if (mSpecification.Format != FramebufferFormat::RG32F)
                    {
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment, 0);
                    }
                }

                if (multisample)
                {
                    glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &mDepthAttachment);
                    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, mDepthAttachment);
                    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, mSpecification.Samples, GL_DEPTH24_STENCIL8, mSpecification.Width, mSpecification.Height, GL_FALSE);
                    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
                }
                else if (mSpecification.Format != FramebufferFormat::RG32F)
                {
                    glCreateTextures(GL_TEXTURE_2D, 1, &mDepthAttachment);
                    glBindTexture(GL_TEXTURE_2D, mDepthAttachment);
                    glTexImage2D(
                        GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, mSpecification.Width, mSpecification.Height, 0,
                        GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL
                    );
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, mDepthAttachment, 0);
                }

                if (mSpecification.Format != FramebufferFormat::RG32F)
                {
                    if (multisample)
                    {
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_ColorAttachment, 0);
                    }
                    else
                    {
                        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_ColorAttachment, 0);
                        if (mSpecification.Format == FramebufferFormat::COMP)
                        {
                            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, m_ColorAttachment2, 0);
                            const GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
                            glDrawBuffers(2, buffers);
                        }
                    }

                    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, mDepthAttachment, 0);
                }
#endif


                NR_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            });
    }

    void GLFrameBuffer::Bind() const
    {
        Ref<const GLFrameBuffer> instance = this;
        Renderer::Submit([instance]() {
            glBindFramebuffer(GL_FRAMEBUFFER, instance->mID);
            glViewport(0, 0, instance->mWidth, instance->mHeight);
            });
    }

    void GLFrameBuffer::Unbind() const
    {
        Renderer::Submit([]() {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            });
    }

    void GLFrameBuffer::BindTexture(uint32_t attachmentIndex, uint32_t slot) const
    {
        Ref<const GLFrameBuffer> instance = this;
        Renderer::Submit([instance, attachmentIndex, slot]() {
            glBindTextureUnit(slot, instance->mColorAttachments[attachmentIndex]);
            });
    }
}