#include "nrpch.h"
#include "GLFramebuffer.h"

#include <glad/glad.h>

namespace NR
{
    static GLenum TextureTarget(bool multisampled)
    {
        return multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
    }

    static void CreateTextures(bool multisampled, uint32_t* outID, uint32_t count)
    {
        glCreateTextures(TextureTarget(multisampled), count, outID);
    }

    static void BindTexture(bool multisapled, uint32_t id)
    {
        glBindTexture(TextureTarget(multisapled), id);
    }

    static void AttachColorTexture(
        uint32_t id,
        int samples,
        GLenum internalFormat,
        GLenum format,
        uint32_t width,
        uint32_t height,
        int index
    )
    {
        bool multisampled = samples > 1;
        if (multisampled)
        {
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalFormat, width, height, GL_FALSE);
        }
        else
        {
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, TextureTarget(multisampled), id, 0);
    }

    static void AttachDepthRenderbuffer(
        uint32_t id,
        GLenum format,
        GLenum attachmentType,
        uint32_t width,
        uint32_t height
    )
    {
        glRenderbufferStorage(GL_RENDERBUFFER, format, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachmentType, GL_RENDERBUFFER, id);
    }

    static bool IsDepthFormat(FramebufferTextureFormat format)
    {
        switch (format)
        {
        case FramebufferTextureFormat::Depth: return true;
        }

        return false;
    }

    GLFramebuffer::GLFramebuffer(const FramebufferStruct& spec)
        :mSpecification(spec)
    {
        for (auto format : mSpecification.Attachments.AttachmentsVector)
        {
            if (IsDepthFormat(format.TextureFormat))
            {
                mDepthAttachmentSpecification = format;
            }
            else
            {
                mTextureAttachmentSpecification.emplace_back(format.TextureFormat);
            }
        }

        Invalidate();
    }

    GLFramebuffer::~GLFramebuffer()
    {
        glDeleteFramebuffers(1, &mID);
        glDeleteTextures(mTextureIDs.size(), mTextureIDs.data());
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
            glDeleteTextures(mTextureIDs.size(), mTextureIDs.data());
            glDeleteRenderbuffers(1, &mDepthID);

            mTextureIDs.clear();
            mDepthID = 0;
        }

        glCreateFramebuffers(1, &mID);
        glBindFramebuffer(GL_FRAMEBUFFER, mID);

        bool multisample = mSpecification.Samples > 1;

        if (mTextureAttachmentSpecification.size())
        {
            mTextureIDs.resize(mTextureAttachmentSpecification.size());
            CreateTextures(multisample, mTextureIDs.data(), mTextureIDs.size());

            for (size_t i = 0; i < mTextureIDs.size(); ++i)
            {
                BindTexture(multisample, mTextureIDs[i]);

                switch (mTextureAttachmentSpecification[i].TextureFormat)
                {
                case FramebufferTextureFormat::RGBA8:
                    AttachColorTexture(
                        mTextureIDs[i],
                        mSpecification.Samples,
                        GL_RGBA8,
                        GL_RGBA,
                        mSpecification.Width,
                        mSpecification.Height, i
                    );
                    break;
                case FramebufferTextureFormat::RED_INTEGER:
                    AttachColorTexture(
                        mTextureIDs[i],
                        mSpecification.Samples,
                        GL_R32I,
                        GL_RED_INTEGER,
                        mSpecification.Width,
                        mSpecification.Height, i
                    );
                    break;
                }
            }
        }

        if (mDepthAttachmentSpecification.TextureFormat != FramebufferTextureFormat::None)
        {
            glGenRenderbuffers(1, &mDepthID);
            glBindRenderbuffer(GL_RENDERBUFFER, mDepthID);
            switch (mDepthAttachmentSpecification.TextureFormat)
            {
            case FramebufferTextureFormat::DEPTH24STENCIL8:
                AttachDepthRenderbuffer(
                    mDepthID,
                    GL_DEPTH24_STENCIL8,
                    GL_DEPTH_STENCIL_ATTACHMENT,
                    mSpecification.Width,
                    mSpecification.Height
                );
                break;
            }
        }

        if (mTextureIDs.size() > 1)
        {
            NR_CORE_ASSERT(mTextureIDs.size() <= 4, "Incomplete Attachments");
            GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
            glDrawBuffers(mTextureIDs.size(), buffers);
        }
        else if(mTextureIDs.empty())
        {
            glDrawBuffer(GL_NONE);
        }

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

    int GLFramebuffer::GetPixel(uint32_t attachmentIndex, int x, int y)
    {
        NR_CORE_ASSERT(attachmentIndex < mTextureIDs.size(), "Index out of Array!");
        glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);

        int pixelData;
        glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);
        return pixelData;
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