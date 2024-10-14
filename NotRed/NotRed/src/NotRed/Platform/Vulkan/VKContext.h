#pragma once

#include "VKDevice.h"
#include "VKSwapChain.h"

#include "NotRed/Renderer/Renderer.h"

struct GLFWwindow;

namespace NR
{
	class VKContext : public RendererContext
	{
	public:
		VKContext();
		~VKContext() override;

		void Init() override;

		Ref<VKDevice> GetDevice() { return mDevice; }

		static VkInstance GetInstance() { return sVKInstance; }

		static Ref<VKContext> Get() { return Ref<VKContext>(Renderer::GetContext()); }
		static Ref<VKDevice> GetCurrentDevice() { return Get()->GetDevice(); }

	private:
		Ref<VKPhysicalDevice> mPhysicalDevice;
		Ref<VKDevice> mDevice;

		inline static VkInstance sVKInstance;
		VkDebugReportCallbackEXT mDebugReportCallback = VK_NULL_HANDLE;
		VkPipelineCache mPipelineCache = nullptr;

		VKSwapChain mSwapChain;
	};
}