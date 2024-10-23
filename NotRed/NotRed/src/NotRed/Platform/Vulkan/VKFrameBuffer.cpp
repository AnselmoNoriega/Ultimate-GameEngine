#include "nrpch.h"
#include "VKFrameBuffer.h"

#include "NotREd/Renderer/Renderer.h"

#include "VKContext.h"

namespace NR
{
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

        uint32_t attachmentIndex = 0;
        if (!mSpecification.ExistingFrameBuffer)
        {
            for (auto& attachmentSpec : mSpecification.Attachments.Attachments)
            {
                if (mSpecification.ExistingImage && mSpecification.ExistingImage->GetSpecification().Deinterleaved)
                {
                    NR_CORE_ASSERT(!Utils::IsDepthFormat(attachmentSpec.Format), "Only supported for color attachments");
                    mAttachmentImages.emplace_back(mSpecification.ExistingImage);
                }
                else if (mSpecification.ExistingImages.find(attachmentIndex) != mSpecification.ExistingImages.end())
                {
                    if (!Utils::IsDepthFormat(attachmentSpec.Format))
                    {
                        mAttachmentImages.emplace_back();
                    }
                }
                else if (Utils::IsDepthFormat(attachmentSpec.Format))
                {
                    ImageSpecification imageSpec;
                    imageSpec.Format = attachmentSpec.Format;
                    imageSpec.Usage = ImageUsage::Attachment;
                    imageSpec.Width = mWidth;
                    imageSpec.Height = mHeight;
                    mDepthAttachmentImage = Image2D::Create(imageSpec);
                }
                else
                {
                    ImageSpecification imageSpec;
                    imageSpec.Format = attachmentSpec.Format;
                    imageSpec.Usage = ImageUsage::Attachment;
                    imageSpec.Width = mWidth;
                    imageSpec.Height = mHeight;
                    mAttachmentImages.emplace_back(Image2D::Create(imageSpec));
                }
                ++attachmentIndex;
            }
        }

        NR_CORE_ASSERT(spec.Attachments.Attachments.size());
        Resize((uint32_t)((float)(mWidth)*spec.Scale), (uint32_t)((float)mHeight * spec.Scale), true);
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
            Invalidate();
        }
        else
        {
            VKSwapChain& swapChain = Application::Get().GetWindow().GetSwapChain();
            mRenderPass = swapChain.GetRenderPass();

            mClearValues.clear();
            mClearValues.emplace_back().color = { 0.0f, 0.0f, 0.0f, 1.0f };
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

    void VKFrameBuffer::Invalidate()
    {
        Ref< VKFrameBuffer> instance = this;
        Renderer::Submit([instance]() mutable
            {
                instance->RT_Invalidate();
            });
    }

    void VKFrameBuffer::RT_Invalidate()
    {
        NR_CORE_TRACE("VKFrameBuffer::RT_Invalidate ({})", mSpecification.DebugName);

        auto device = VKContext::GetCurrentDevice()->GetVulkanDevice();

        if (mFrameBuffer)
        {
            VkFramebuffer frameBuffer = mFrameBuffer;
            Renderer::SubmitResourceFree([frameBuffer]()
                {
                    const auto device = VKContext::GetCurrentDevice()->GetVulkanDevice();
                    vkDestroyFramebuffer(device, frameBuffer, nullptr);
                });

            if (!mSpecification.ExistingFrameBuffer)
            {
                uint32_t attachmentIndex = 0;
                for (Ref<VKImage2D> image : mAttachmentImages)
                {
                    if (mSpecification.ExistingImages.find(attachmentIndex) != mSpecification.ExistingImages.end())
                    {
                        continue;
                    }

                    if (!image->GetSpecification().Deinterleaved || attachmentIndex == 0 && !image->GetLayerImageView(0))
                    {
                        image->Release();
                    }
                    ++attachmentIndex;
                }

                if (mDepthAttachmentImage)
                {
                    if (mSpecification.ExistingImages.find((uint32_t)mSpecification.Attachments.Attachments.size() - 1) == mSpecification.ExistingImages.end())
                    {
                        mDepthAttachmentImage->Release();
                    }
                }
            }
        }

        VKAllocator allocator("FrameBuffer");

        std::vector<VkAttachmentDescription> attachmentDescriptions;
        attachmentDescriptions.reserve(mAttachmentImages.size());

        std::vector<VkAttachmentReference> colorAttachmentReferences;
        VkAttachmentReference depthAttachmentReference;

        mClearValues.resize(mSpecification.Attachments.Attachments.size());

        bool createImages = mAttachmentImages.empty();
        bool createDepthImage = !(bool)mDepthAttachmentImage;

        if (mSpecification.ExistingFrameBuffer)
        {
            mAttachmentImages.clear();
        }

        uint32_t attachmentIndex = 0;
        for (auto attachmentSpec : mSpecification.Attachments.Attachments)
        {
            if (Utils::IsDepthFormat(attachmentSpec.Format))
            {
                if (mSpecification.ExistingImage)
                {
                    mDepthAttachmentImage = mSpecification.ExistingImage;
                }
                else if (mSpecification.ExistingFrameBuffer)
                {
                    Ref<VKFrameBuffer> existingFrameBuffer = mSpecification.ExistingFrameBuffer.As<VKFrameBuffer>();
                    mDepthAttachmentImage = existingFrameBuffer->GetDepthImage();
                }
                else if (mSpecification.ExistingImages.find(attachmentIndex) != mSpecification.ExistingImages.end())
                {
                    Ref<Image2D> existingImage = mSpecification.ExistingImages.at(attachmentIndex);
                    NR_CORE_ASSERT(Utils::IsDepthFormat(existingImage->GetSpecification().Format), "Trying to attach non-depth image as depth attachment");
                    mDepthAttachmentImage = existingImage;
                }
                else
                {
                    Ref<VKImage2D> depthAttachmentImage = mDepthAttachmentImage.As<VKImage2D>();
                    auto& spec = depthAttachmentImage->GetSpecification();
                    spec.Width = mWidth;
                    spec.Height = mHeight;
                    depthAttachmentImage->RT_Invalidate(); // Create immediately
                }

                VkAttachmentDescription& attachmentDescription = attachmentDescriptions.emplace_back();
                attachmentDescription.flags = 0;
                attachmentDescription.format = Utils::VulkanImageFormat(attachmentSpec.Format);
                attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
                attachmentDescription.loadOp = mSpecification.ClearOnLoad ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
                attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachmentDescription.initialLayout = mSpecification.ClearOnLoad ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

                if (attachmentSpec.Format == ImageFormat::DEPTH24STENCIL8 || true)
                {
                    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                    depthAttachmentReference = { attachmentIndex, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
                }
                else
                {
                    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
                    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
                    depthAttachmentReference = { attachmentIndex, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL };
                }
                mClearValues[attachmentIndex].depthStencil = { 1.0f, 0 };
            }
            else
            {
                Ref<VKImage2D> colorAttachment;
                if (mSpecification.ExistingFrameBuffer)
                {
                    Ref<VKFrameBuffer> existingFramebuffer = mSpecification.ExistingFrameBuffer.As<VKFrameBuffer>();
                    Ref<Image2D> existingImage = existingFramebuffer->GetImage(attachmentIndex);
                    colorAttachment = mAttachmentImages.emplace_back(existingImage).As<VKImage2D>();
                }
                else if (mSpecification.ExistingImages.find(attachmentIndex) != mSpecification.ExistingImages.end())
                {
                    Ref<Image2D> existingImage = mSpecification.ExistingImages[attachmentIndex];
                    NR_CORE_ASSERT(!Utils::IsDepthFormat(existingImage->GetSpecification().Format), "Trying to attach depth image as color attachment");
                    colorAttachment = existingImage.As<VKImage2D>();
                    mAttachmentImages[attachmentIndex] = existingImage;
                }
                else
                {
                    if (createImages)
                    {
                        ImageSpecification spec;
                        spec.Format = attachmentSpec.Format;
                        spec.Usage = ImageUsage::Attachment;
                        spec.Width = mWidth;
                        spec.Height = mHeight;
                        colorAttachment = mAttachmentImages.emplace_back(Image2D::Create(spec)).As<VKImage2D>();
                        NR_CORE_VERIFY(false);
                    }
                    else
                    {
                        Ref<Image2D> image = mAttachmentImages[attachmentIndex];
                        ImageSpecification& spec = image->GetSpecification();
                        spec.Width = mWidth;
                        spec.Height = mHeight;
                        colorAttachment = image.As<VKImage2D>();
                        if (!colorAttachment->GetSpecification().Deinterleaved)
                        {
                            colorAttachment->RT_Invalidate(); // Create immediately
                        }
                        else if (attachmentIndex == 0 && mSpecification.ExistingImageLayers[0] == 0)// Only invalidate the first layer from only the first framebuffer
                        {
                            colorAttachment->RT_Invalidate(); // Create immediately
                            colorAttachment->RT_CreatePerSpecificLayerImageViews(mSpecification.ExistingImageLayers);
                        }
                        else if (attachmentIndex == 0)
                        {
                            colorAttachment->RT_CreatePerSpecificLayerImageViews(mSpecification.ExistingImageLayers);
                        }
                    }
                }

                VkAttachmentDescription& attachmentDescription = attachmentDescriptions.emplace_back();
                attachmentDescription.flags = 0;
                attachmentDescription.format = Utils::VulkanImageFormat(attachmentSpec.Format);
                attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
                attachmentDescription.loadOp = mSpecification.ClearOnLoad ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
                attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachmentDescription.initialLayout = mSpecification.ClearOnLoad ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                const auto& clearColor = mSpecification.ClearColor;
                mClearValues[attachmentIndex].color = { {clearColor.r, clearColor.g, clearColor.b, clearColor.a} };
                colorAttachmentReferences.emplace_back(VkAttachmentReference{ attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
            }

            attachmentIndex++;
        }

        VkSubpassDescription subpassDescription = {};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = uint32_t(colorAttachmentReferences.size());
        subpassDescription.pColorAttachments = colorAttachmentReferences.data();
        if (mDepthAttachmentImage)
        {
            subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
        }

        std::vector<VkSubpassDependency> dependencies;

        if (mAttachmentImages.size())
        {
            {
                VkSubpassDependency& depedency = dependencies.emplace_back();
                depedency.srcSubpass = VK_SUBPASS_EXTERNAL;
                depedency.dstSubpass = 0;
                depedency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                depedency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                depedency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                depedency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                depedency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            }
            {
                VkSubpassDependency& depedency = dependencies.emplace_back();
                depedency.srcSubpass = 0;
                depedency.dstSubpass = VK_SUBPASS_EXTERNAL;
                depedency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                depedency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                depedency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                depedency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                depedency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            }
        }
        if (mDepthAttachmentImage)
        {
            {
                VkSubpassDependency& depedency = dependencies.emplace_back();
                depedency.srcSubpass = VK_SUBPASS_EXTERNAL;
                depedency.dstSubpass = 0;
                depedency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                depedency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                depedency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                depedency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                depedency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            }
            {
                VkSubpassDependency& depedency = dependencies.emplace_back();
                depedency.srcSubpass = 0;
                depedency.dstSubpass = VK_SUBPASS_EXTERNAL;
                depedency.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                depedency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                depedency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                depedency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                depedency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            }
        }

        // Create the actual renderpass
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
        renderPassInfo.pAttachments = attachmentDescriptions.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpassDescription;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &mRenderPass));

        std::vector<VkImageView> attachments(mAttachmentImages.size());
        for (uint32_t i = 0; i < mAttachmentImages.size(); ++i)
        {
            Ref<VKImage2D> image = mAttachmentImages[i].As<VKImage2D>();
            if (image->GetSpecification().Deinterleaved)
            {
                attachments[i] = image->GetLayerImageView(mSpecification.ExistingImageLayers[i]);
                NR_CORE_ASSERT(attachments[i]);
            }
            else
            {
                attachments[i] = image->GetImageInfo().ImageView;
                NR_CORE_ASSERT(attachments[i]);
            }
        }

        if (mDepthAttachmentImage)
        {
            Ref<VKImage2D> image = mDepthAttachmentImage.As<VKImage2D>();
            if (mSpecification.ExistingImage)
            {
                NR_CORE_ASSERT(mSpecification.ExistingImageLayers.size() == 1, "Depth attachments do not support deinterleaving");
                attachments.emplace_back(image->GetLayerImageView(mSpecification.ExistingImageLayers[0]));
                NR_CORE_ASSERT(attachments.back());
            }
            else
            {
                attachments.emplace_back(image->GetImageInfo().ImageView);
                NR_CORE_ASSERT(attachments.back());
            }
        }

        VkFramebufferCreateInfo frameBufferCreateInfo = {};
        frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferCreateInfo.renderPass = mRenderPass;
        frameBufferCreateInfo.attachmentCount = uint32_t(attachments.size());
        frameBufferCreateInfo.pAttachments = attachments.data();
        frameBufferCreateInfo.width = mWidth;
        frameBufferCreateInfo.height = mHeight;
        frameBufferCreateInfo.layers = 1;
        VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &mFrameBuffer));
    }
}