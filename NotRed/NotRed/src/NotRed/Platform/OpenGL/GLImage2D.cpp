#include "nrpch.h"
#include "GLImage.h"

#include "NotRed/Renderer/Renderer.h"

namespace NR
{
    GLImage2D::GLImage2D(ImageFormat format, uint32_t width, uint32_t height, const void* data)
        : mWidth(width), mHeight(height), mFormat(format)
    {
        if (data)
        {
            mImageData = Buffer::Copy(data, Utils::GetImageMemorySize(format, width, height));
        }
    }

    GLImage2D::GLImage2D(ImageFormat format, uint32_t width, uint32_t height, Buffer buffer)
        : mWidth(width), mHeight(height), mFormat(format), mImageData(buffer)
    {
    }

    GLImage2D::~GLImage2D()
    {
        mImageData.Release();
        if (mID)
        {
            RendererID rendererID = mID;
            Renderer::Submit([rendererID]()
                {
                    glDeleteTextures(1, &rendererID);
                });
        }
    }

    void GLImage2D::Invalidate()
    {
        if (mID)
        {
            Release();
        }

        glCreateTextures(GL_TEXTURE_2D, 1, &mID);

        GLenum internalFormat = Utils::OpenGLImageInternalFormat(mFormat);
        uint32_t mipCount = Utils::CalculateMipCount(mWidth, mHeight);
        glTextureStorage2D(mID, mipCount, internalFormat, mWidth, mHeight);
        if (mImageData)
        {
            GLenum format = Utils::OpenGLImageFormat(mFormat);
            GLenum dataType = Utils::OpenGLFormatDataType(mFormat);
            glTextureSubImage2D(mID, 0, 0, 0, mWidth, mHeight, format, dataType, mImageData.Data);
            glGenerateTextureMipmap(mID);
        }

        glCreateSamplers(1, &mSamplerRendererID);
        glSamplerParameteri(mSamplerRendererID, GL_TEXTURE_MIN_FILTER, mImageData ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glSamplerParameteri(mSamplerRendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glSamplerParameteri(mSamplerRendererID, GL_TEXTURE_WRAP_R, GL_REPEAT);
        glSamplerParameteri(mSamplerRendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glSamplerParameteri(mSamplerRendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    void GLImage2D::Release()
    {
        if (mID)
        {
            glDeleteTextures(1, &mID);
            mID = 0;
        }
        mImageData.Release();
    }
}