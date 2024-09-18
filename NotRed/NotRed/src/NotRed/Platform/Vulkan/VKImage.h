#pragma once

#include "vulkan/vulkan.h"

#include "NotRed/Renderer/Image.h"

namespace NR
{
    struct VKImageInfo
    {
        VkImage Image;
        VkImageView ImageView;
        VkSampler Sampler;
        VkDeviceMemory Memory;
    };

    class VKImage2D : public Image2D
    {
    public:
        VKImage2D(ImageFormat format, uint32_t width, uint32_t height);
        ~VKImage2D() override;

        void Invalidate() override;
        void Release() override;

        uint32_t GetWidth() const override { return mWidth; }
        uint32_t GetHeight() const override { return mHeight; }

        ImageFormat GetFormat() const override { return mFormat; }

        VKImageInfo& GetImageInfo() { return mInfo; }
        const VKImageInfo& GetImageInfo() const { return mInfo; }

        const VkDescriptorImageInfo& GetDescriptor() { return mDescriptorImageInfo; }

        virtual Buffer GetBuffer() const override { return mImageData; }
        virtual Buffer& GetBuffer() override { return mImageData; }

        virtual uint64_t GetHash() const override { return (uint64_t)mInfo.Image; }

        void UpdateDescriptor();

    private:
        ImageFormat mFormat;
        uint32_t mWidth = 0, mHeight = 0;

        Buffer mImageData;

        VKImageInfo mInfo;
        VkDescriptorImageInfo mDescriptorImageInfo = {};
    };

    namespace Utils 
    {
        inline VkFormat VulkanImageFormat(ImageFormat format)
        {
            switch (format)
            {
            case ImageFormat::RGBA:  return VK_FORMAT_R8G8B8A8_UNORM;
            case ImageFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
            default:
                NR_CORE_ASSERT(false);
                return VK_FORMAT_UNDEFINED;
            }
        }
    }
}