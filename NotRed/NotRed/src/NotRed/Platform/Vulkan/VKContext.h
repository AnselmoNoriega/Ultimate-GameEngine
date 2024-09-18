#pragma once

#include "Vulkan.h"
#include "VKDevice.h"
#include "VKAllocator.h"
#include "VKSwapChain.h"

#include "NotRed/Renderer/Renderer.h"

struct GLFWwindow;

namespace NR
{
	class VKContext : public RendererContext
	{
	public:
		VKContext(GLFWwindow* windowHandle);
		~VKContext() override;

		void Create() override;
		void SwapBuffers() override;

		void Resize(uint32_t width, uint32_t height) override;

		void BeginFrame() override;

		Ref<VKDevice> GetDevice() { return mDevice; }
		VKSwapChain& GetSwapChain() { return mSwapChain; }

		static VkInstance GetInstance() { return sVKInstance; }

		static Ref<VKContext> Get() { return Ref<VKContext>(Renderer::GetContext()); }
		static Ref<VKDevice> GetCurrentDevice() { return Get()->GetDevice(); }

	private:
		GLFWwindow* mWindowHandle;

		Ref<VKPhysicalDevice> mPhysicalDevice;
		Ref<VKDevice> mDevice;

		inline static VkInstance sVKInstance;
		VkDebugReportCallbackEXT mDebugReportCallback = VK_NULL_HANDLE;
		VkPipelineCache mPipelineCache = nullptr;

		VKAllocator mAllocator;
		VKSwapChain mSwapChain;
	};
}