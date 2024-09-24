#include "nrpch.h"
#include "GLImage.h"

#include "NotRed/Renderer/Renderer.h"

namespace NR
{
    GLImage2D::GLImage2D(ImageSpecification specification, const void* data)
        : mSpecification(specification), mWidth(specification.Width), mHeight(specification.Height)
    {
        if (data)
        {
            mImageData = Buffer::Copy(data, Utils::GetImageMemorySize(specification.Format, specification.Width, specification.Height));
        }
    }

    GLImage2D::GLImage2D(ImageSpecification specification, Buffer buffer)
        : mSpecification(specification), mWidth(specification.Width), mHeight(specification.Height), mImageData(buffer)
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

        GLenum internalFormat = Utils::OpenGLImageInternalFormat(mSpecification.Format);
        uint32_t mipCount = Utils::CalculateMipCount(mWidth, mHeight);
        glTextureStorage2D(mID, mipCount, internalFormat, mWidth, mHeight);
        if (mImageData)
        {
            GLenum format = Utils::OpenGLImageFormat(mSpecification.Format);
            GLenum dataType = Utils::OpenGLFormatDataType(mSpecification.Format);
            glTextureSubImage2D(mID, 0, 0, 0, mWidth, mHeight, format, dataType, mImageData.Data);
            glGenerateTextureMipmap(mID);
        }
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

    void GLImage2D::CreateSampler(TextureProperties properties)
    {
        glCreateSamplers(1, &mSamplerRendererID);
        glSamplerParameteri(mSamplerRendererID, GL_TEXTURE_MIN_FILTER, Utils::OpenGLSamplerFilter(properties.SamplerFilter, properties.GenerateMips));
        glSamplerParameteri(mSamplerRendererID, GL_TEXTURE_MAG_FILTER, Utils::OpenGLSamplerFilter(properties.SamplerFilter, false));
        glSamplerParameteri(mSamplerRendererID, GL_TEXTURE_WRAP_R, Utils::OpenGLSamplerWrap(properties.SamplerWrap));
        glSamplerParameteri(mSamplerRendererID, GL_TEXTURE_WRAP_S, Utils::OpenGLSamplerWrap(properties.SamplerWrap));
        glSamplerParameteri(mSamplerRendererID, GL_TEXTURE_WRAP_T, Utils::OpenGLSamplerWrap(properties.SamplerWrap));
    }
}