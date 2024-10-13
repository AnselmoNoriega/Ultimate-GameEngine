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

		NR_CORE_ASSERT(spec.Attachments.Attachments.size());
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
			Invalidate();
		}
		else
		{
			VKSwapChain& swapChain = Application::Get().GetWindow().GetSwapChain();
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
		NR_CORE_TRACE("VKFrameBuffer::RT_Invalidate");

		auto device = VKContext::GetCurrentDevice()->GetVulkanDevice();

		if (mFrameBuffer)
		{
			VkFramebuffer framebuffer = mFrameBuffer;
			Renderer::SubmitResourceFree([framebuffer]()
				{
					auto device = VKContext::GetCurrentDevice()->GetVulkanDevice();
					vkDestroyFramebuffer(device, framebuffer, nullptr);
				});

			for (auto image : mAttachmentImages)
			{
				image->Release();
			}

			if (mDepthAttachmentImage)
			{
				mDepthAttachmentImage->Release();
			}
		}

		VKAllocator allocator("FrameBuffer");

		std::vector<VkAttachmentDescription> attachmentDescriptions;
		attachmentDescriptions.reserve(mAttachmentImages.size());

		std::vector<VkAttachmentReference> colorAttachmentReferences;
		VkAttachmentReference depthAttachmentReference;

		mClearValues.resize(mSpecification.Attachments.Attachments.size());

		bool createImages = mAttachmentImages.empty();

		uint32_t attachmentIndex = 0;
		for (auto attachmentSpec : mSpecification.Attachments.Attachments)
		{
			if (Utils::IsDepthFormat(attachmentSpec.Format))
			{
				if (!mSpecification.ExistingImage)
				{
					if (createImages)
					{
						ImageSpecification spec;
						spec.Format = attachmentSpec.Format;
						spec.Usage = ImageUsage::Attachment;
						spec.Width = mWidth;
						spec.Height = mHeight;
						mDepthAttachmentImage = Image2D::Create(spec);
					}
					else
					{
						ImageSpecification& spec = mDepthAttachmentImage->GetSpecification();
						spec.Width = mWidth;
						spec.Height = mHeight;
					}

					Ref<VKImage2D> depthAttachmentImage = mDepthAttachmentImage.As<VKImage2D>();
					depthAttachmentImage->RT_Invalidate();
				}
				else
				{
					mDepthAttachmentImage = mSpecification.ExistingImage;
				}

				VkAttachmentDescription& attachmentDescription = attachmentDescriptions.emplace_back();
				attachmentDescription.flags = 0;
				attachmentDescription.format = Utils::VulkanImageFormat(attachmentSpec.Format);
				attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
				attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
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
				NR_CORE_ASSERT(!mSpecification.ExistingImage, "Not supported for color attachments");

				Ref<VKImage2D> colorAttachment;
				if (createImages)
				{
					ImageSpecification spec;
					spec.Format = attachmentSpec.Format;
					spec.Usage = ImageUsage::Attachment;
					spec.Width = mWidth;
					spec.Height = mHeight;
					colorAttachment = mAttachmentImages.emplace_back(Image2D::Create(spec)).As<VKImage2D>();
				}
				else
				{
					Ref<Image2D> image = mAttachmentImages[attachmentIndex];
					ImageSpecification& spec = image->GetSpecification();
					spec.Width = mWidth;
					spec.Height = mHeight;
					colorAttachment = image.As<VKImage2D>();
				}

				colorAttachment->RT_Invalidate(); // Create immediately

				VkAttachmentDescription& attachmentDescription = attachmentDescriptions.emplace_back();
				attachmentDescription.flags = 0;
				attachmentDescription.format = Utils::VulkanImageFormat(attachmentSpec.Format);
				attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
				attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				const auto& clearColor = mSpecification.ClearColor;
				mClearValues[attachmentIndex].color = { {clearColor.r, clearColor.g, clearColor.b, clearColor.a} };
				colorAttachmentReferences.emplace_back(VkAttachmentReference{ attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
			}

			attachmentIndex++;
		}

		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = colorAttachmentReferences.size();
		subpassDescription.pColorAttachments = colorAttachmentReferences.data();
		if (mDepthAttachmentImage)
		{
			subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
		}

		std::array<VkSubpassDependency, 2> dependencies;

#if 0
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
#endif

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

		VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &mRenderPass));

		std::vector<VkImageView> attachments(mAttachmentImages.size());
		for (uint32_t i = 0; i < mAttachmentImages.size(); i++)
		{
			Ref<VKImage2D> image = mAttachmentImages[i].As<VKImage2D>();
			attachments[i] = image->GetImageInfo().ImageView;
		}

		if (mDepthAttachmentImage)
		{
			Ref<VKImage2D> image = mDepthAttachmentImage.As<VKImage2D>();
			if (mSpecification.ExistingImage)
			{
				attachments.emplace_back(image->GetLayerImageView(mSpecification.ExistingImageLayer));
			}
			else
			{
				attachments.emplace_back(image->GetImageInfo().ImageView);
			}
		}

		VkFramebufferCreateInfo frameBufferCreateInfo = {};
		frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCreateInfo.renderPass = mRenderPass;
		frameBufferCreateInfo.attachmentCount = attachments.size();
		frameBufferCreateInfo.pAttachments = attachments.data();
		frameBufferCreateInfo.width = mWidth;
		frameBufferCreateInfo.height = mHeight;
		frameBufferCreateInfo.layers = 1;

		VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &mFrameBuffer));
	}
}