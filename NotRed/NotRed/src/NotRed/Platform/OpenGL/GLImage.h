#pragma once

#include "NotRed/Renderer/Image.h"
#include "NotRed/Renderer/RendererTypes.h"

#include <glad/glad.h>

namespace NR
{
    class GLImage2D : public Image2D
    {
    public:
        GLImage2D(ImageSpecification specification, Buffer buffer);
        GLImage2D(ImageSpecification specification, const void* data = nullptr);
        ~GLImage2D() override;

        void Invalidate() override;
        void Release() override;

        void CreateSampler(TextureProperties properties);

        void CreatePerLayerImageViews() override {}

        uint32_t GetWidth() const override { return mWidth; }
        uint32_t GetHeight() const override { return mHeight; }

        ImageSpecification& GetSpecification() override { return mSpecification; }
        const ImageSpecification& GetSpecification() const override { return mSpecification; }

        Buffer GetBuffer() const override { return mImageData; }
        Buffer& GetBuffer() override { return mImageData; }

        RendererID& GetRendererID() { return mID; }
        RendererID GetRendererID() const { return mID; }
        uint64_t GetHash() const override { return (uint64_t)mID; }

        RendererID& GetSamplerRendererID() { return mSamplerRendererID; }
        RendererID GetSamplerRendererID() const { return mSamplerRendererID; }

    private:
        RendererID mID = 0;
        RendererID mSamplerRendererID = 0;

        ImageSpecification mSpecification;
        uint32_t mWidth, mHeight;

        Buffer mImageData;
    };

    namespace Utils
    {
        inline GLenum OpenGLImageFormat(ImageFormat format)
        {
            switch (format)
            {
            case ImageFormat::RGB:     return GL_RGB;
            case ImageFormat::SRGB:    return GL_RGB;
            case ImageFormat::RGBA:
            case ImageFormat::RGBA16F:
            case ImageFormat::RGBA32F: return GL_RGBA;
            default:
                NR_CORE_ASSERT(false, "Unknown image format");
                return 0;
            }
        }

        inline GLenum OpenGLImageInternalFormat(ImageFormat format)
        {
            switch (format)
            {
            case ImageFormat::RGB:             return GL_RGB8;
            case ImageFormat::SRGB:            return GL_SRGB8;
            case ImageFormat::RGBA:            return GL_RGBA8;
            case ImageFormat::RGBA16F:         return GL_RGBA16F;
            case ImageFormat::RGBA32F:         return GL_RGBA32F;
            case ImageFormat::DEPTH24STENCIL8: return GL_DEPTH24_STENCIL8;
            case ImageFormat::DEPTH32F:        return GL_DEPTH_COMPONENT32F;
            default:
                NR_CORE_ASSERT(false, "Unknown image format");
                return 0;
            }
        }

        inline GLenum OpenGLFormatDataType(ImageFormat format)
        {
            switch (format)
            {
            case ImageFormat::RGB:
            case ImageFormat::SRGB:
            case ImageFormat::RGBA:    return GL_UNSIGNED_BYTE;
            case ImageFormat::RGBA16F:
            case ImageFormat::RGBA32F: return GL_FLOAT;
            default:
                NR_CORE_ASSERT(false, "Unknown image format");
                return 0;
            }
        }

        inline GLenum OpenGLSamplerWrap(TextureWrap wrap)
        {
            switch (wrap)
            {
            case TextureWrap::Clamp:   return GL_CLAMP_TO_EDGE;
            case TextureWrap::Repeat:  return GL_REPEAT;
            default:
                NR_CORE_ASSERT(false, "Unknown wrap mode");
                return 0;
            }
        }

        inline GLenum OpenGLSamplerFilter(TextureFilter filter, bool mipmap)
        {
            switch (filter)
            {
            case TextureFilter::Linear:   return mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
            case TextureFilter::Nearest:  return mipmap ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
            default:
                NR_CORE_ASSERT(false, "Unknown wrap mode");
                return 0;
            }
        }
    }
}