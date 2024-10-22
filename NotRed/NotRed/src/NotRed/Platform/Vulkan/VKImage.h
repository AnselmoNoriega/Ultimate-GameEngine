#pragma once

#include "vulkan/vulkan.h"

#include "VulkanMemoryAllocator/vk_mem_alloc.h"
#include "NotRed/Platform/Vulkan/VKContext.h"
#include "NotRed/Renderer/Image.h"

namespace NR
{
    struct VKImageInfo
    {
        VkImage Image = nullptr;
        VkImageView ImageView = nullptr;
        VkSampler Sampler = nullptr;
        VmaAllocation MemoryAlloc = nullptr;
    };

    class VKImage2D : public Image2D
    {
    public:
        VKImage2D(ImageSpecification specification);
        ~VKImage2D() override;

        void Invalidate() override;
        void Release() override;

        uint32_t GetWidth() const override { return mSpecification.Width; }
        uint32_t GetHeight() const override { return mSpecification.Height; }

        ImageSpecification& GetSpecification() override { return mSpecification; }
        const ImageSpecification& GetSpecification() const override { return mSpecification; }

        void RT_Invalidate();
        void CreatePerLayerImageViews() override;
        void RT_CreatePerLayerImageViews();

        void RT_CreatePerSpecificLayerImageViews(const std::vector<uint32_t>& layerIndices);

        virtual VkImageView GetLayerImageView(uint32_t layer)
        {
            NR_CORE_ASSERT(layer < mPerLayerImageViews.size());
            return mPerLayerImageViews[layer];
        }

        VKImageInfo& GetImageInfo() { return mInfo; }
        const VKImageInfo& GetImageInfo() const { return mInfo; }

        const VkDescriptorImageInfo& GetDescriptor() { return mDescriptorImageInfo; }

        virtual Buffer GetBuffer() const override { return mImageData; }
        virtual Buffer& GetBuffer() override { return mImageData; }

        virtual uint64_t GetHash() const override { return (uint64_t)mInfo.Image; }

        void UpdateDescriptor();

    private:
        ImageSpecification mSpecification;

        Buffer mImageData;

        VKImageInfo mInfo;
        std::vector<VkImageView> mPerLayerImageViews;
        VkDescriptorImageInfo mDescriptorImageInfo = {};
    };

    namespace Utils
    {
        inline VkFormat VulkanImageFormat(ImageFormat format)
        {
            switch (format)
            {
            case ImageFormat::RED32F:            return VK_FORMAT_R32_SFLOAT;
            case ImageFormat::RG16F:		     return VK_FORMAT_R16G16_SFLOAT;
            case ImageFormat::RG32F:		     return VK_FORMAT_R32G32_SFLOAT;
            case ImageFormat::RGBA:              return VK_FORMAT_R8G8B8A8_UNORM;
            case ImageFormat::RGBA16F:           return VK_FORMAT_R16G16B16A16_SFLOAT;
            case ImageFormat::RGBA32F:           return VK_FORMAT_R32G32B32A32_SFLOAT;
            case ImageFormat::DEPTH32F:          return VK_FORMAT_D32_SFLOAT;
            case ImageFormat::DEPTH24STENCIL8:   return VKContext::GetCurrentDevice()->GetPhysicalDevice()->GetDepthFormat();
            default:
                NR_CORE_ASSERT(false);
                return VK_FORMAT_UNDEFINED;
            }
        }
    }
}