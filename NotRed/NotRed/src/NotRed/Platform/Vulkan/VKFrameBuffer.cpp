#include "nrpch.h"
#include "VKFrameBuffer.h"

#include "VKContext.h"

#include "NotRed/Renderer/Renderer.h"

namespace NR
{
    namespace Utils
    {
        static bool IsDepthFormat(ImageFormat format)
        {
            switch (format)
            {
            case ImageFormat::DEPTH24STENCIL8:
            case ImageFormat::DEPTH32F:
                return true;
            default:
                return false;
            }
        }
    }


    VKFrameBuffer::VKFrameBuffer(const FrameBufferSpecification& spec)
        : mSpecification(spec)
    {
        if (spec.Width == 0)
        {
            mWidth = Application::Get().GetWindow().GetWidth();
            mHeight = Application::Get().GetWindow().GetHeight();
        }
        else
        {
            mWidth = spec.Width;
            mHeight = spec.Height;
        }

        NR_CORE_ASSERT(spec.Attachments.Attachments.size());
        for (auto format : mSpecification.Attachments.Attachments)
        {
            if (!Utils::IsDepthFormat(format.Format))
            {
                mAttachments.emplace_back(Image2D::Create(ImageFormat::RGBA32F, mWidth, mHeight));
            }
            else
            {
                mDepthAttachment = Image2D::Create(format.Format, mWidth, mHeight);
            }
        }

        Resize(mWidth * spec.Scale, mHeight * spec.Scale, true);
    }

    VKFrameBuffer::~VKFrameBuffer()
    {
    }

    void VKFrameBuffer::Resize(uint32_t width, uint32_t height, bool forceRecreate)
    {
        if (!forceRecreate && (mWidth == width && mHeight == height))
        {
            return;
        }

        mWidth = width;
        mHeight = height;
        if (!mSpecification.SwapChainTarget)
        {
            Ref<VKFrameBuffer> instance = this;
            Renderer::Submit([instance, width, height]() mutable
                {
                    auto device = VKContext::GetCurrentDevice()->GetVulkanDevice();

                    if (instance->mFrameBuffer)
                    {
                        vkDestroyFramebuffer(device, instance->mFrameBuffer, nullptr);
                        for (auto image : instance->mAttachments)
                        {
                            image->Release();
                        }

                        if (instance->mDepthAttachment)
                        {
                            instance->mDepthAttachment->Release();
                        }
                    }

                    VKAllocator allocator("FrameBuffer");

                    std::vector<VkAttachmentDescription> attachmentDescriptions;
                    attachmentDescriptions.reserve(instance->mAttachments.size());

                    std::vector<VkAttachmentReference> colorAttachmentReferences(instance->mAttachments.size());
                    VkAttachmentReference depthAttachmentReference;

                    uint32_t attachmentCount = instance->mAttachments.size() + (instance->mDepthAttachment ? 1 : 0);
                    instance->mClearValues.resize(attachmentCount);

                    // Color attachments
                    uint32_t attachmentIndex = 0;
                    for (auto image : instance->mAttachments)
                    {
                        const VkFormat COLOR_BUFFER_FORMAT = VK_FORMAT_R32G32B32A32_SFLOAT;

                        Ref<VKImage2D> colorAttachment = image.As<VKImage2D>();
                        auto& info = colorAttachment->GetImageInfo();

                        VkImageCreateInfo imageCreateInfo = {};
                        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
                        imageCreateInfo.format = COLOR_BUFFER_FORMAT;
                        imageCreateInfo.extent.width = width;
                        imageCreateInfo.extent.height = height;
                        imageCreateInfo.extent.depth = 1;
                        imageCreateInfo.mipLevels = 1;
                        imageCreateInfo.arrayLayers = 1;
                        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                        // We will sample directly from the color attachment
                        imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
                        VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &info.Image));

                        VkMemoryRequirements memReqs;
                        vkGetImageMemoryRequirements(device, info.Image, &memReqs);

                        allocator.Allocate(memReqs, &info.Memory);

                        VK_CHECK_RESULT(vkBindImageMemory(device, info.Image, info.Memory, 0));

                        VkImageViewCreateInfo colorImageViewCreateInfo = {};
                        colorImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                        colorImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                        colorImageViewCreateInfo.format = COLOR_BUFFER_FORMAT;
                        colorImageViewCreateInfo.subresourceRange = {};
                        colorImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        colorImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
                        colorImageViewCreateInfo.subresourceRange.levelCount = 1;
                        colorImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
                        colorImageViewCreateInfo.subresourceRange.layerCount = 1;
                        colorImageViewCreateInfo.image = info.Image;
                        VK_CHECK_RESULT(vkCreateImageView(device, &colorImageViewCreateInfo, nullptr, &info.ImageView));

                        VkSamplerCreateInfo samplerCreateInfo = {};
                        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                        samplerCreateInfo.maxAnisotropy = 1.0f;
                        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
                        samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
                        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                        samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
                        samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
                        samplerCreateInfo.mipLodBias = 0.0f;
                        samplerCreateInfo.maxAnisotropy = 1.0f;
                        samplerCreateInfo.minLod = 0.0f;
                        samplerCreateInfo.maxLod = 1.0f;
                        samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
                        VK_CHECK_RESULT(vkCreateSampler(device, &samplerCreateInfo, nullptr, &info.Sampler));

                        VkAttachmentDescription& attachmentDescription = attachmentDescriptions.emplace_back();
                        attachmentDescription.flags = 0;
                        attachmentDescription.format = COLOR_BUFFER_FORMAT;
                        attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
                        attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                        attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                        attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                        attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                        attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                        attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                        colorAttachment->UpdateDescriptor();

                        const auto& clearColor = instance->mSpecification.ClearColor;
                        instance->mClearValues[attachmentIndex].color = { {clearColor.r, clearColor.g, clearColor.b, clearColor.a} };
                        colorAttachmentReferences[attachmentIndex] = { attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
                        ++attachmentIndex;
                    }

                    if (instance->mDepthAttachment)
                    {
                        VkFormat depthFormat = instance->mDepthAttachment->GetFormat() == ImageFormat::DEPTH32F ? VK_FORMAT_D32_SFLOAT : VKContext::GetCurrentDevice()->GetPhysicalDevice()->GetDepthFormat();

                        auto& info = instance->mDepthAttachment.As<VKImage2D>()->GetImageInfo();

                        VkImageCreateInfo imageCreateInfo = {};
                        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
                        imageCreateInfo.format = depthFormat;
                        imageCreateInfo.extent.width = width;
                        imageCreateInfo.extent.height = height;
                        imageCreateInfo.extent.depth = 1;
                        imageCreateInfo.mipLevels = 1;
                        imageCreateInfo.arrayLayers = 1;
                        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                        imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

                        VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &info.Image));
                        VkMemoryRequirements memoryRequirements;
                        vkGetImageMemoryRequirements(device, info.Image, &memoryRequirements);
                        allocator.Allocate(memoryRequirements, &info.Memory);

                        VK_CHECK_RESULT(vkBindImageMemory(device, info.Image, info.Memory, 0));

                        VkImageViewCreateInfo depthStencilImageViewCreateInfo = {};
                        depthStencilImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                        depthStencilImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                        depthStencilImageViewCreateInfo.format = depthFormat;
                        depthStencilImageViewCreateInfo.flags = 0;
                        depthStencilImageViewCreateInfo.subresourceRange = {};
                        depthStencilImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                        if (instance->mDepthAttachment->GetFormat() == ImageFormat::DEPTH24STENCIL8)
                        {
                            depthStencilImageViewCreateInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                        }
                        depthStencilImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
                        depthStencilImageViewCreateInfo.subresourceRange.levelCount = 1;
                        depthStencilImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
                        depthStencilImageViewCreateInfo.subresourceRange.layerCount = 1;
                        depthStencilImageViewCreateInfo.image = info.Image;
                        VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilImageViewCreateInfo, nullptr, &info.ImageView));

                        VkSamplerCreateInfo samplerCreateInfo = {};
                        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                        samplerCreateInfo.maxAnisotropy = 1.0f;
                        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
                        samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
                        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                        samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
                        samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
                        samplerCreateInfo.mipLodBias = 0.0f;
                        samplerCreateInfo.maxAnisotropy = 1.0f;
                        samplerCreateInfo.minLod = 0.0f;
                        samplerCreateInfo.maxLod = 1.0f;
                        samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
                        VK_CHECK_RESULT(vkCreateSampler(device, &samplerCreateInfo, nullptr, &info.Sampler));

                        VkAttachmentDescription& attachmentDescription = attachmentDescriptions.emplace_back();
                        attachmentDescription.flags = 0;
                        attachmentDescription.format = depthFormat;
                        attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
                        attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                        attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE; 
                        attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                        attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                        attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                        attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                        attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

                        Ref<VKImage2D> image = instance->mDepthAttachment.As<VKImage2D>();
                        image->UpdateDescriptor();

                        depthAttachmentReference = { attachmentIndex, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

                        instance->mClearValues[attachmentIndex].depthStencil = { 1.0f, 0 };
                    }

                    VkSubpassDescription subpassDescription = {};
                    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                    subpassDescription.colorAttachmentCount = colorAttachmentReferences.size();
                    subpassDescription.pColorAttachments = colorAttachmentReferences.data();
                    if (instance->mDepthAttachment)
                    {
                        subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
                    }

                    std::array<VkSubpassDependency, 2> dependencies;
                    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
                    dependencies[0].dstSubpass = 0;
                    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                    dependencies[1].srcSubpass = 0;
                    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
                    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                    // Create the actual renderpass
                    VkRenderPassCreateInfo renderPassInfo = {};
                    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
                    renderPassInfo.pAttachments = attachmentDescriptions.data();
                    renderPassInfo.subpassCount = 1;
                    renderPassInfo.pSubpasses = &subpassDescription;
                    renderPassInfo.dependencyCount = 0;
                    renderPassInfo.pDependencies = nullptr;
                    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
                    renderPassInfo.pDependencies = dependencies.data();

                    VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &instance->mRenderPass));

                    std::vector<VkImageView> attachments(instance->mAttachments.size());
                    for (uint32_t i = 0; i < instance->mAttachments.size(); ++i)
                    {
                        Ref<VKImage2D> image = instance->mAttachments[i].As<VKImage2D>();
                        attachments[i] = image->GetImageInfo().ImageView;
                    }

                    if (instance->mDepthAttachment)
                    {
                        Ref<VKImage2D> image = instance->mDepthAttachment.As<VKImage2D>();
                        attachments.emplace_back(image->GetImageInfo().ImageView);
                    }

                    VkFramebufferCreateInfo framebufferCreateInfo = {};
                    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                    framebufferCreateInfo.renderPass = instance->mRenderPass;
                    framebufferCreateInfo.attachmentCount = attachments.size();
                    framebufferCreateInfo.pAttachments = attachments.data();
                    framebufferCreateInfo.width = width;
                    framebufferCreateInfo.height = height;
                    framebufferCreateInfo.layers = 1;

                    VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &instance->mFrameBuffer));
                });
        }
        else
        {
            VKSwapChain& swapChain = VKContext::Get()->GetSwapChain();
            mRenderPass = swapChain.GetRenderPass();
        }

        for (auto& callback : mResizeCallbacks)
        {
            callback(this);
        }
    }

    void VKFrameBuffer::AddResizeCallback(const std::function<void(Ref<FrameBuffer>)>& func)
    {
        mResizeCallbacks.push_back(func);
    }

}