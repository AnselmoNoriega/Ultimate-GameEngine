#include "nrpch.h"
#include "VKTexture.h"

#include "stb_image.h"

#include "VKImage.h"
#include "VKContext.h"
#include "VKRenderer.h"

namespace NR
{
    namespace Utils
    {
        static VkSamplerAddressMode VulkanSamplerWrap(TextureWrap wrap)
        {
            switch (wrap)
            {
            case TextureWrap::Clamp:   return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case TextureWrap::Repeat:  return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            default:
                NR_CORE_ASSERT(false, "Unknown wrap mode");
                return (VkSamplerAddressMode)0;
            }
        }
        static VkFilter VulkanSamplerFilter(TextureFilter filter)
        {
            switch (filter)
            {
            case TextureFilter::Linear:   return VK_FILTER_LINEAR;
            case TextureFilter::Nearest:  return VK_FILTER_NEAREST;
            default:
                NR_CORE_ASSERT(false, "Unknown filter");
                return (VkFilter)0;
            }
        }

        static size_t GetMemorySize(ImageFormat format, uint32_t width, uint32_t height)
        {
            switch (format)
            {
            case ImageFormat::RG32F:                    return width * height * 2 * sizeof(float);
            case ImageFormat::RED32F:                   return width * height * sizeof(float);
            case ImageFormat::RED8UNormalized:          return width * height * sizeof(std::byte);
            case ImageFormat::RGBA:                     return width * height * 4;
            case ImageFormat::RGBA32F:                  return width * height * 4 * sizeof(float);
            default:
                NR_CORE_ASSERT(false);
                return 0;
            }
        }
    }

    // Texture2D----------------------------------------------------------

    VKTexture2D::VKTexture2D(const std::string& path, TextureProperties properties)
        : mPath(path), mProperties(properties)
    {
        int width, height, channels;

        if (stbi_is_hdr(path.c_str()))
        {
            mImageData.Data = (byte*)stbi_loadf(path.c_str(), &width, &height, &channels, 4);
            mImageData.Size = width * height * 4 * sizeof(float);
            mFormat = ImageFormat::RGBA32F;
        }
        else
        {
            //stbi_set_flip_vertically_on_load(1);
            mImageData.Data = stbi_load(path.c_str(), &width, &height, &channels, 4);
            mImageData.Size = width * height * 4;
            mFormat = ImageFormat::RGBA;
        }

        NR_CORE_ASSERT(mImageData.Data, "Failed to load image!");

        if (!mImageData.Data)
        {
            return;
        }

        mWidth = width;
        mHeight = height;

        ImageSpecification imageSpec;
        imageSpec.Format = mFormat;
        imageSpec.Width = mWidth;
        imageSpec.Height = mHeight;
        imageSpec.Mips = GetMipLevelCount();
        if (properties.Storage)
        {
            imageSpec.Usage = ImageUsage::Storage;
        }
        mImage = Image2D::Create(imageSpec);

        NR_CORE_ASSERT(mFormat != ImageFormat::None);

        Ref<VKTexture2D> instance = this;
        Renderer::Submit([instance]() mutable
            {
                instance->Invalidate();
            });
    }

    VKTexture2D::VKTexture2D(ImageFormat format, uint32_t width, uint32_t height, const void* data, TextureProperties properties)
        : mProperties(properties), mFormat(format)
    {
        mWidth = width;
        mHeight = height;

        uint32_t size = Utils::GetMemorySize(format, width, height);

        if (data)
        {
            mImageData = Buffer::Copy(data, size);
            memcpy(mImageData.Data, data, mImageData.Size);
        }

        ImageSpecification imageSpec;
        imageSpec.Format = mFormat;
        imageSpec.Width = mWidth;
        imageSpec.Height = mHeight;
        imageSpec.Mips = GetMipLevelCount();
        mImage = Image2D::Create(imageSpec);

        Ref<VKTexture2D> instance = this;
        Renderer::Submit([instance]() mutable
            {
                instance->Invalidate();
            });
    }

    VKTexture2D::~VKTexture2D()
    {
        if (mImage)
        {
            mImage->Release();
        }
    }

    void VKTexture2D::Resize(uint32_t width, uint32_t height)
    {
        mWidth = width;
        mHeight = height;

        Ref<VKTexture2D> instance = this;
        Renderer::Submit([instance]() mutable
            {
                instance->Invalidate();
            });
    }

    void VKTexture2D::Invalidate()
    {
        auto device = VKContext::GetCurrentDevice();
        auto vulkanDevice = device->GetVulkanDevice();

        mImage->Release();

        uint32_t mipCount = GetMipLevelCount();

        ImageSpecification& imageSpec = mImage->GetSpecification();
        imageSpec.Format = mFormat;
        imageSpec.Width = mWidth;
        imageSpec.Height = mHeight;
        imageSpec.Mips = mipCount;
        if (!mImageData)
        {
            imageSpec.Usage = ImageUsage::Storage;
        }

        Ref<VKImage2D> image = mImage.As<VKImage2D>();
        image->RT_Invalidate();

        auto& info = image->GetImageInfo();

        if (mImageData)
        {
            VkDeviceSize size = mImageData.Size;

            VkMemoryAllocateInfo memAllocInfo{};
            memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

            VKAllocator allocator("Texture2D");

            // Create staging buffer
            VkBufferCreateInfo bufferCreateInfo{};
            bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCreateInfo.size = size;
            bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            VkBuffer stagingBuffer;
            VmaAllocation stagingBufferAllocation = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer);

            // Copy data to staging buffer
            uint8_t* destData = allocator.MapMemory<uint8_t>(stagingBufferAllocation);
            NR_CORE_ASSERT(mImageData.Data);
            memcpy(destData, mImageData.Data, size);
            allocator.UnmapMemory(stagingBufferAllocation);

            VkCommandBuffer copyCmd = device->GetCommandBuffer(true);

            // Image memory barriers for the texture image

            // The sub resource range describes the regions of the 
            // image that will be transitioned using the memory barriers below
            VkImageSubresourceRange subresourceRange = {};
            // Image only contains color data
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            // Start at first mip level
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = 1;
            subresourceRange.layerCount = 1;

            // Transition the texture image layout to transfer target, so we can safely copy our buffer data to it.
            VkImageMemoryBarrier imageMemoryBarrier{};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.image = info.Image;
            imageMemoryBarrier.subresourceRange = subresourceRange;
            imageMemoryBarrier.srcAccessMask = 0;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

            // Insert a memory dependency at the proper pipeline stages that will execute the image layout transition 
            // Source pipeline stage is host write/read exection (VK_PIPELINE_STAGE_HOST_BIT)
            // Destination pipeline stage is copy command exection (VK_PIPELINE_STAGE_TRANSFER_BIT)
            vkCmdPipelineBarrier(
                copyCmd,
                VK_PIPELINE_STAGE_HOST_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);

            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel = 0;
            bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
            bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageExtent.width = mWidth;
            bufferCopyRegion.imageExtent.height = mHeight;
            bufferCopyRegion.imageExtent.depth = 1;
            bufferCopyRegion.bufferOffset = 0;

            // Copy mip levels from staging buffer
            vkCmdCopyBufferToImage(
                copyCmd,
                stagingBuffer,
                info.Image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &bufferCopyRegion);

            Utils::InsertImageMemoryBarrier(copyCmd, info.Image,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                subresourceRange);

            device->FlushCommandBuffer(copyCmd);

            // Clean up staging resources
            allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);
        }
        else
        {
            VkCommandBuffer transitionCommandBuffer = device->GetCommandBuffer(true);

            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.layerCount = 1;
            subresourceRange.levelCount = GetMipLevelCount();

            Utils::SetImageLayout(transitionCommandBuffer, info.Image, VK_IMAGE_LAYOUT_UNDEFINED, image->GetDescriptor().imageLayout, subresourceRange);
            device->FlushCommandBuffer(transitionCommandBuffer);
        }

        // CREATE TEXTURE SAMPLER --------------------------------------------------------------------
        // Create a texture sampler
        VkSamplerCreateInfo sampler{};
        sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler.maxAnisotropy = 1.0f;
        sampler.magFilter = Utils::VulkanSamplerFilter(mProperties.SamplerFilter);
        sampler.minFilter = Utils::VulkanSamplerFilter(mProperties.SamplerFilter);
        sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler.addressModeU = Utils::VulkanSamplerWrap(mProperties.SamplerWrap);
        sampler.addressModeV = Utils::VulkanSamplerWrap(mProperties.SamplerWrap);
        sampler.addressModeW = Utils::VulkanSamplerWrap(mProperties.SamplerWrap);
        sampler.mipLodBias = 0.0f;
        sampler.compareOp = VK_COMPARE_OP_NEVER;
        sampler.minLod = 0.0f;
        sampler.maxLod = (float)mipCount;
        // Enable anisotropic filtering

        sampler.maxAnisotropy = 1.0;
        sampler.anisotropyEnable = VK_FALSE;
        sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        VK_CHECK_RESULT(vkCreateSampler(vulkanDevice, &sampler, nullptr, &info.Sampler));

        if (!mProperties.Storage)
        {
            VkImageViewCreateInfo view{};
            view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view.format = Utils::VulkanImageFormat(mFormat);
            view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

            // The subresource range describes the set of mip levels (and array layers) that can be accessed through this image view
            // It's possible to create multiple image views for a single image referring to different (and/or overlapping) ranges of the image
            view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            view.subresourceRange.baseMipLevel = 0;
            view.subresourceRange.baseArrayLayer = 0;
            view.subresourceRange.layerCount = 1;
            view.subresourceRange.levelCount = mipCount;
            view.image = info.Image;
            VK_CHECK_RESULT(vkCreateImageView(vulkanDevice, &view, nullptr, &info.ImageView));
            image->UpdateDescriptor();
        }

        if (mImageData)
        {
            GenerateMips();
        }
    }

    void VKTexture2D::Bind(uint32_t slot) const
    {
    }

    void VKTexture2D::Lock()
    {
    }

    void VKTexture2D::Unlock()
    {
    }

    Buffer VKTexture2D::GetWriteableBuffer()
    {
        return mImageData;
    }

    const std::string& VKTexture2D::GetPath() const
    {
        return mPath;
    }

    uint32_t VKTexture2D::GetMipLevelCount() const
    {
        return Utils::CalculateMipCount(mWidth, mHeight);
    }

    std::pair<uint32_t, uint32_t> VKTexture2D::GetMipSize(uint32_t mip) const
    {
        uint32_t width = mWidth;
        uint32_t height = mHeight;

        while (mip != 0)
        {
            width /= 2;
            height /= 2;
            mip--;
        }

        return { width, height };
    }

    void VKTexture2D::GenerateMips()
    {
        auto device = VKContext::GetCurrentDevice();
        auto vulkanDevice = device->GetVulkanDevice();

        Ref<VKImage2D> image = mImage.As<VKImage2D>();
        const auto& info = image->GetImageInfo();

        const VkCommandBuffer blitCmd = VKContext::GetCurrentDevice()->GetCommandBuffer(true);

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = info.Image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        const auto mipLevels = GetMipLevelCount();
        for (uint32_t i = 1; i < mipLevels; ++i)
        {
            VkImageBlit imageBlit{};

            // Source
            imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBlit.srcSubresource.layerCount = 1;
            imageBlit.srcSubresource.mipLevel = i - 1;
            imageBlit.srcOffsets[1].x = int32_t(mWidth >> (i - 1));
            imageBlit.srcOffsets[1].y = int32_t(mHeight >> (i - 1));
            imageBlit.srcOffsets[1].z = 1;

            // Destination
            imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBlit.dstSubresource.layerCount = 1;
            imageBlit.dstSubresource.mipLevel = i;
            imageBlit.dstOffsets[1].x = int32_t(mWidth >> i);
            imageBlit.dstOffsets[1].y = int32_t(mHeight >> i);
            imageBlit.dstOffsets[1].z = 1;

            VkImageSubresourceRange mipSubRange = {};
            mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            mipSubRange.baseMipLevel = i;
            mipSubRange.levelCount = 1;
            mipSubRange.layerCount = 1;

            // Prepare current mip level as image blit destination
            Utils::InsertImageMemoryBarrier(blitCmd, info.Image,
                0, VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                mipSubRange);

            // Blit from previous level
            vkCmdBlitImage(
                blitCmd,
                info.Image,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                info.Image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &imageBlit,
                VK_FILTER_LINEAR);

            // Prepare current mip level as image blit source for next level
            Utils::InsertImageMemoryBarrier(blitCmd, info.Image,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                mipSubRange);
        }

        // After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.layerCount = 1;
        subresourceRange.levelCount = mipLevels;

        Utils::InsertImageMemoryBarrier(blitCmd, info.Image,
            VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            subresourceRange);

        VKContext::GetCurrentDevice()->FlushCommandBuffer(blitCmd);
    }

    // TextureCube---------------------------------------------------------------------

    VKTextureCube::VKTextureCube(ImageFormat format, uint32_t width, uint32_t height, const void* data, TextureProperties properties)
        : mFormat(format), mWidth(width), mHeight(height), mProperties(properties)
    {
        if (data)
        {
            uint32_t size = width * height * 4 * 6; // six layers
            mLocalStorage = Buffer::Copy(data, size);
        }

        Ref<VKTextureCube> instance = this;
        Renderer::Submit([instance]() mutable
            {
                instance->Invalidate();
            });
    }

    VKTextureCube::VKTextureCube(const std::string& path, TextureProperties properties)
        : mProperties(properties)
    {
    }

    VKTextureCube::~VKTextureCube()
    {
        VkImageView imageView = mDescriptorImageInfo.imageView;
        VkSampler sampler = mDescriptorImageInfo.sampler;
        VkImage image = mImage;
        VmaAllocation allocation = mMemoryAlloc;
        Renderer::SubmitResourceFree([imageView, sampler, image, allocation]()
            {
                NR_CORE_TRACE("Destroying VulkanTextureCube");
                auto vulkanDevice = VKContext::GetCurrentDevice()->GetVulkanDevice();
                vkDestroyImageView(vulkanDevice, imageView, nullptr);
                vkDestroySampler(vulkanDevice, sampler, nullptr);
                VKAllocator allocator("TextureCube");
                allocator.DestroyImage(image, allocation);
            });
    }

    void VKTextureCube::Invalidate()
    {
        auto device = VKContext::GetCurrentDevice();
        auto vulkanDevice = device->GetVulkanDevice();

        VkFormat format = Utils::VulkanImageFormat(mFormat);
        uint32_t mipCount = GetMipLevelCount();

        VkMemoryAllocateInfo memAllocInfo{};
        memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

        VKAllocator allocator("TextureCube");

        // Create optimal tiled target image on the device
        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = format;
        imageCreateInfo.mipLevels = mipCount;
        imageCreateInfo.arrayLayers = 6;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.extent = { mWidth, mHeight, 1 };
        imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
        imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        mMemoryAlloc = allocator.AllocateImage(imageCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY, mImage);

        mDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        // Copy data if present
        if (mLocalStorage)
        {
            // Create staging buffer
            VkBufferCreateInfo bufferCreateInfo{};
            bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCreateInfo.size = mLocalStorage.Size;
            bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            VkBuffer stagingBuffer;
            VmaAllocation stagingBufferAllocation = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer);

            // Copy data to staging buffer
            uint8_t* destData = allocator.MapMemory<uint8_t>(stagingBufferAllocation);
            memcpy(destData, mLocalStorage.Data, mLocalStorage.Size);
            allocator.UnmapMemory(stagingBufferAllocation);

            VkCommandBuffer copyCmd = device->GetCommandBuffer(true);

            // Image memory barriers for the texture image

            // The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
            VkImageSubresourceRange subresourceRange = {};
            // Image only contains color data
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            // Start at first mip level
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = 1;
            subresourceRange.layerCount = 6;

            // Transition the texture image layout to transfer target, so we can safely copy our buffer data to it.
            VkImageMemoryBarrier imageMemoryBarrier{};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.image = mImage;
            imageMemoryBarrier.subresourceRange = subresourceRange;
            imageMemoryBarrier.srcAccessMask = 0;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

            // Insert a memory dependency at the proper pipeline stages that will execute the image layout transition 
            // Source pipeline stage is host write/read exection (VK_PIPELINE_STAGE_HOST_BIT)
            // Destination pipeline stage is copy command exection (VK_PIPELINE_STAGE_TRANSFER_BIT)
            vkCmdPipelineBarrier(
                copyCmd,
                VK_PIPELINE_STAGE_HOST_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);

            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel = 0;
            bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
            bufferCopyRegion.imageSubresource.layerCount = 6;
            bufferCopyRegion.imageExtent.width = mWidth;
            bufferCopyRegion.imageExtent.height = mHeight;
            bufferCopyRegion.imageExtent.depth = 1;
            bufferCopyRegion.bufferOffset = 0;

            // Copy mip levels from staging buffer
            vkCmdCopyBufferToImage(
                copyCmd,
                stagingBuffer,
                mImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &bufferCopyRegion);

            Utils::InsertImageMemoryBarrier(copyCmd, mImage,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                subresourceRange);

            device->FlushCommandBuffer(copyCmd);

            // Clean up staging resources
            allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);
        }

        VkCommandBuffer layoutCmd = device->GetCommandBuffer(true);

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = mipCount;
        subresourceRange.layerCount = 6;

        Utils::SetImageLayout(
            layoutCmd, mImage,
            VK_IMAGE_LAYOUT_UNDEFINED,
            mDescriptorImageInfo.imageLayout,
            subresourceRange);

        device->FlushCommandBuffer(layoutCmd);

        // CREATE TEXTURE SAMPLER----------------------------------------------------
        // Create a texture sampler
        VkSamplerCreateInfo sampler{};
        sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler.maxAnisotropy = 1.0f;
        sampler.magFilter = Utils::VulkanSamplerFilter(mProperties.SamplerFilter);
        sampler.minFilter = Utils::VulkanSamplerFilter(mProperties.SamplerFilter);
        sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler.addressModeU = Utils::VulkanSamplerWrap(mProperties.SamplerWrap);
        sampler.addressModeV = Utils::VulkanSamplerWrap(mProperties.SamplerWrap);
        sampler.addressModeW = Utils::VulkanSamplerWrap(mProperties.SamplerWrap);
        sampler.mipLodBias = 0.0f;
        sampler.compareOp = VK_COMPARE_OP_NEVER;
        sampler.minLod = 0.0f;
        // Set max level-of-detail to mip level count of the texture
        sampler.maxLod = (float)mipCount;
        // Enable anisotropic filtering
        // This feature is optional, so we must check if it's supported on the device

        sampler.maxAnisotropy = 1.0;
        sampler.anisotropyEnable = VK_FALSE;
        sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        VK_CHECK_RESULT(vkCreateSampler(vulkanDevice, &sampler, nullptr, &mDescriptorImageInfo.sampler));

        // Create image view
        // Textures are not directly accessed by the shaders and
        // are abstracted by image views containing additional
        // information and sub resource ranges
        VkImageViewCreateInfo view{};
        view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        view.format = format;
        view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
        // The subresource range describes the set of mip levels (and array layers) that can be accessed through this image view
        // It's possible to create multiple image views for a single image referring to different (and/or overlapping) ranges of the image
        view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view.subresourceRange.baseMipLevel = 0;
        view.subresourceRange.baseArrayLayer = 0;
        view.subresourceRange.layerCount = 6;
        view.subresourceRange.levelCount = mipCount;
        view.image = mImage;
        VK_CHECK_RESULT(vkCreateImageView(vulkanDevice, &view, nullptr, &mDescriptorImageInfo.imageView));
    }

    uint32_t VKTextureCube::GetMipLevelCount() const
    {
        return Utils::CalculateMipCount(mWidth, mHeight);
    }

    std::pair<uint32_t, uint32_t> VKTextureCube::GetMipSize(uint32_t mip) const
    {
        uint32_t width = mWidth;
        uint32_t height = mHeight;

        while (mip != 0)
        {
            width /= 2;
            height /= 2;
            mip--;
        }

        return { width, height };
    }

    VkImageView VKTextureCube::CreateImageViewSingleMip(uint32_t mip)
    {
        auto device = VKContext::GetCurrentDevice();
        auto vulkanDevice = device->GetVulkanDevice();

        VkFormat format = Utils::VulkanImageFormat(mFormat);

        VkImageViewCreateInfo view{};
        view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        view.format = format;
        view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
        view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view.subresourceRange.baseMipLevel = mip;
        view.subresourceRange.baseArrayLayer = 0;
        view.subresourceRange.layerCount = 6;
        view.subresourceRange.levelCount = 1;
        view.image = mImage;

        VkImageView result;
        VK_CHECK_RESULT(vkCreateImageView(vulkanDevice, &view, nullptr, &result));
        return result;
    }

    void VKTextureCube::GenerateMips(bool readonly)
    {
        auto device = VKContext::GetCurrentDevice();
        auto vulkanDevice = device->GetVulkanDevice();

        VkCommandBuffer blitCmd = VKContext::GetCurrentDevice()->GetCommandBuffer(true);

        uint32_t mipLevels = GetMipLevelCount();
        for (uint32_t face = 0; face < 6; ++face)
        {
            VkImageSubresourceRange mipSubRange = {};
            mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            mipSubRange.baseMipLevel = 0;
            mipSubRange.baseArrayLayer = face;
            mipSubRange.levelCount = 1;
            mipSubRange.layerCount = 1;

            // Prepare current mip level as image blit destination
            Utils::InsertImageMemoryBarrier(blitCmd, mImage,
                0, VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                mipSubRange);
        }

        for (uint32_t i = 1; i < mipLevels; ++i)
        {
            for (uint32_t face = 0; face < 6; ++face)
            {
                VkImageBlit imageBlit{};

                // Source
                imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.srcSubresource.layerCount = 1;
                imageBlit.srcSubresource.mipLevel = i - 1;
                imageBlit.srcSubresource.baseArrayLayer = face;
                imageBlit.srcOffsets[1].x = int32_t(mWidth >> (i - 1));
                imageBlit.srcOffsets[1].y = int32_t(mHeight >> (i - 1));
                imageBlit.srcOffsets[1].z = 1;

                // Destination
                imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.dstSubresource.layerCount = 1;
                imageBlit.dstSubresource.mipLevel = i;
                imageBlit.dstSubresource.baseArrayLayer = face;
                imageBlit.dstOffsets[1].x = int32_t(mWidth >> i);
                imageBlit.dstOffsets[1].y = int32_t(mHeight >> i);
                imageBlit.dstOffsets[1].z = 1;

                VkImageSubresourceRange mipSubRange = {};
                mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                mipSubRange.baseMipLevel = i;
                mipSubRange.baseArrayLayer = face;
                mipSubRange.levelCount = 1;
                mipSubRange.layerCount = 1;

                // Prepare current mip level as image blit destination
                Utils::InsertImageMemoryBarrier(blitCmd, mImage,
                    0, VK_ACCESS_TRANSFER_WRITE_BIT,
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    mipSubRange);

                // Blit from previous level
                vkCmdBlitImage(
                    blitCmd,
                    mImage,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    mImage,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &imageBlit,
                    VK_FILTER_LINEAR);

                // Prepare current mip level as image blit source for next level
                Utils::InsertImageMemoryBarrier(blitCmd, mImage,
                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    mipSubRange);
            }
        }

        // After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.layerCount = 6;
        subresourceRange.levelCount = mipLevels;

        Utils::InsertImageMemoryBarrier(blitCmd, mImage,
            VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readonly ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            subresourceRange);

        VKContext::GetCurrentDevice()->FlushCommandBuffer(blitCmd);

        mMipsGenerated = true;

        mDescriptorImageInfo.imageLayout = readonly ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
    }
}