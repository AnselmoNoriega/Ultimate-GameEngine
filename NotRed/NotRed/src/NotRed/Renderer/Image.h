#pragma once

#include "NotRed/Core/Core.h"
#include "NotRed/Core/Buffer.h"

namespace NR
{
    enum class ImageFormat
    {
        None,
        RGB,
        RGBA,
        RGBA16F,
        RGBA32F,
        RG32F,

        SRGB,

        DEPTH32F,
        DEPTH24STENCIL8,

        Depth = DEPTH24STENCIL8
    };
    enum class ImageUsage
    {
        None,
        Texture,
        Attachment
    };
    enum class TextureWrap
    {
        None,
        Clamp,
        Repeat
    };
    enum class TextureFilter
    {
        None,
        Linear,
        Nearest
    };
    enum class TextureType
    {
        None,
        Texture2D,
        TextureCube
    };
    struct ImageSpecification
    {
        ImageFormat Format = ImageFormat::RGBA;
        ImageUsage Usage = ImageUsage::Texture;
        uint32_t Width = 0;
        uint32_t Height = 0;
        uint32_t Mips = 1;
        uint32_t Layers = 1;
    };

    struct TextureProperties
    {
        TextureWrap SamplerWrap = TextureWrap::Repeat;
        TextureFilter SamplerFilter = TextureFilter::Linear;
        bool GenerateMips = true;
        bool StandardRGB = false;
    };

    class Image : public RefCounted
    {
    public:
        virtual ~Image() {}

        virtual void Invalidate() = 0;
        virtual void Release() = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        virtual ImageSpecification& GetSpecification() = 0;
        virtual const ImageSpecification& GetSpecification() const = 0;

        virtual Buffer GetBuffer() const = 0;
        virtual Buffer& GetBuffer() = 0;

        virtual void CreatePerLayerImageViews() = 0;

        virtual uint64_t GetHash() const = 0;
    };

    class Image2D : public Image
    {
    public:
        static Ref<Image2D> Create(ImageSpecification specification, Buffer buffer);
        static Ref<Image2D> Create(ImageSpecification specification, const void* data = nullptr);
    };

    namespace Utils 
    {
        inline uint32_t GetImageFormatBPP(ImageFormat format)
        {
            switch (format)
            {
            case ImageFormat::RGB:
            case ImageFormat::SRGB:         return 3;
            case ImageFormat::RGBA:         return 4;
            case ImageFormat::RGBA16F:      return 2 * 4;
            case ImageFormat::RGBA32F:      return 4 * 4;
            default:
            {
                NR_CORE_ASSERT(false);
                return 0;
            }
            }
        }

        inline uint32_t CalculateMipCount(uint32_t width, uint32_t height)
        {
            return std::floor(std::log2(glm::min(width, height))) + 1;
        }

        inline uint32_t GetImageMemorySize(ImageFormat format, uint32_t width, uint32_t height)
        {
            return width * height * GetImageFormatBPP(format);
        }

        inline bool IsDepthFormat(ImageFormat format)
        {
            if (format == ImageFormat::DEPTH24STENCIL8 || format == ImageFormat::DEPTH32F)
            {
                return true;
            }
            return false;
        }
    }
}