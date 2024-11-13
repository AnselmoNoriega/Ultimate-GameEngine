#include "nrpch.h"
#include "GLFrameBuffer.h"

#include <glad/glad.h>

#include "NotRed/Renderer/Renderer.h"
#include "GLImage.h"

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
                NR_CORE_ASSERT(false, "Unknown format!");
                return 0;
            }
        }

        static GLenum DepthAttachmentType(ImageFormat format)
        {
            switch (format)
            {
            case ImageFormat::DEPTH32F:             return GL_DEPTH_ATTACHMENT;
            case ImageFormat::DEPTH24STENCIL8:      return GL_DEPTH_STENCIL_ATTACHMENT;
            default:
                NR_CORE_ASSERT(false, "Unknown format");
                return 0;
            }
        }

        static Ref<Image2D> CreateAndAttachColorAttachment(int samples, ImageFormat format, uint32_t width, uint32_t height, int index)
        {
            bool multisampled = samples > 1;
            Ref<Image2D> image;
            if (multisampled)
            {
                //glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
            }
            else
            {
                ImageSpecification spec;
                spec.Format = format;
                spec.Width = width;
                spec.Height = height;
                image = Image2D::Create(spec);
                image->Invalidate();
            }

            Ref<GLImage2D> glImage = image.As<GLImage2D>();
            glImage->CreateSampler(TextureProperties());
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, TextureTarget(multisampled), glImage->GetRendererID(), 0);
            return image;
        }

        static Ref<Image2D> AttachDepthTexture(int samples, ImageFormat format, uint32_t width, uint32_t height)
        {
            bool multisampled = samples > 1;
            Ref<Image2D> image;
            if (multisampled)
            {
                //glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
            }
            else
            {
                ImageSpecification spec;
                spec.Format = format;
                spec.Width = width;
                spec.Height = height;
                image = Image2D::Create(spec);
                image->Invalidate();
            }

            Ref<GLImage2D> glImage = image.As<GLImage2D>();
            glImage->CreateSampler(TextureProperties());
            glFramebufferTexture2D(GL_FRAMEBUFFER, Utils::DepthAttachmentType(format), TextureTarget(multisampled), glImage->GetRendererID(), 0);
            return image;
        }
    }

    GLFrameBuffer::GLFrameBuffer(const FrameBufferSpecification& spec)
        : mSpecification(spec), mWidth(spec.Width), mHeight(spec.Height)
    { 
        NR_CORE_ASSERT(spec.Attachments.Attachments.size());

        for (auto format : mSpecification.Attachments.Attachments)
        {
            if (Utils::IsDepthFormat(format.Format))
            {
                mDepthAttachmentFormat = format.Format;
            }
            else
            {
                mColorAttachmentFormats.emplace_back(format.Format);
            }
        }

        uint32_t width = spec.Width;
        uint32_t height = spec.Height;
        if (width == 0)
        {
            width = Application::Get().GetWindow().GetWidth();
            height = Application::Get().GetWindow().GetHeight();
        }

        Resize(width, height, true);
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

                    instance->mColorAttachments.clear();
                }

                glGenFramebuffers(1, &instance->mID);
                glBindFramebuffer(GL_FRAMEBUFFER, instance->mID);

                if (instance->mColorAttachmentFormats.size())
                {
                    instance->mColorAttachments.resize(instance->mColorAttachmentFormats.size());

                    for (size_t i = 0; i < instance->mColorAttachments.size(); ++i)
                    {
                        instance->mColorAttachments[i] = 
                            Utils::CreateAndAttachColorAttachment(
                                instance->mSpecification.Samples, 
                                instance->mColorAttachmentFormats[i],
                                instance->mWidth, instance->mHeight, (int)i);
                    }
                }

                if (instance->mDepthAttachmentFormat != ImageFormat::None)
                {
                    instance->mDepthAttachment = Utils::AttachDepthTexture(instance->mSpecification.Samples, instance->mDepthAttachmentFormat, instance->mWidth, instance->mHeight);
                }

                if (instance->mColorAttachments.size() > 1)
                {
                    NR_CORE_ASSERT(instance->mColorAttachments.size() <= 4);
                    GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
                    glDrawBuffers((int)instance->mColorAttachments.size(), buffers);
                }
                else if (instance->mColorAttachments.empty())
                {
                    // Only depth-pass
                    glDrawBuffer(GL_NONE);
                }

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