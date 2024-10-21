#include "nrpch.h"
#include "VKContext.h"

#include <glfw/glfw3.h>

#include "Vulkan.h"

namespace NR
{
#ifdef NR_DEBUG
	static bool sValidation = true;
#else
	static bool sValidation = true;
#endif

	static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
	{
		(void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
		//TODO: Handle when closeing Window
		//NR_CORE_WARN("VulkanDebugCallback:\n  Object Type: {0}\n  Message: {1}", (const char*)objectType, pMessage);
		return VK_FALSE;
	}

	VKContext::VKContext()
	{
	}

	VKContext::~VKContext()
	{
		mDevice->Destroy();

		vkDestroyInstance(sVKInstance, nullptr);
		sVKInstance = nullptr;
	}

	void VKContext::Init()
	{
		NR_CORE_INFO("VKContext::Create");

		NR_CORE_ASSERT(glfwVulkanSupported(), "GLFW must support Vulkan!");

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Not-Red";
		appInfo.pEngineName = "Not-Red";
		appInfo.apiVersion = VK_API_VERSION_1_2;

		// Extensions and Validation----------------------------------------
#define VK_KHR_WIN32_SURFACE_EXTENSION_NAME "VK_KHR_win32_surface"
		std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
		if (sValidation)
		{
			instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
			instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		}

		VkValidationFeatureEnableEXT enables[] = { VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT };
		VkValidationFeaturesEXT features = {};
		features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
		features.enabledValidationFeatureCount = 1;
		features.pEnabledValidationFeatures = enables;

		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pNext = nullptr;
		instanceCreateInfo.pApplicationInfo = &appInfo;
		instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
		instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

		if (sValidation)
		{
			const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
			uint32_t instanceLayerCount;
			vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
			std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
			vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
			bool validationLayerPresent = false;
			NR_CORE_TRACE("Vulkan Instance Layers:");
			for (const VkLayerProperties& layer : instanceLayerProperties)
			{
				NR_CORE_TRACE("  {0}", layer.layerName);
				if (strcmp(layer.layerName, validationLayerName) == 0)
				{
					validationLayerPresent = true;
					break;
				}
			}
			if (validationLayerPresent)
			{
				instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
				instanceCreateInfo.enabledLayerCount = 1;
			}
			else
			{
				NR_CORE_ERROR("Validation layer VK_LAYER_LUNARG_standard_validation not present, validation is disabled");
			}
		}

		VK_CHECK_RESULT(vkCreateInstance(&instanceCreateInfo, nullptr, &sVKInstance));

		if (sValidation)
		{
			auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(sVKInstance, "vkCreateDebugReportCallbackEXT");
			NR_CORE_ASSERT(vkCreateDebugReportCallbackEXT != NULL, "");
			VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
			debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
			debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
			debug_report_ci.pfnCallback = VulkanDebugReportCallback;
			debug_report_ci.pUserData = NULL;
			VK_CHECK_RESULT(vkCreateDebugReportCallbackEXT(sVKInstance, &debug_report_ci, nullptr, &mDebugReportCallback));
		}

		mPhysicalDevice = VKPhysicalDevice::Select();

		VkPhysicalDeviceFeatures enabledFeatures;
		memset(&enabledFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
		enabledFeatures.samplerAnisotropy = true;
		enabledFeatures.wideLines = true;
		enabledFeatures.fillModeNonSolid = true;
		mDevice = Ref<VKDevice>::Create(mPhysicalDevice, enabledFeatures);

		VKAllocator::Init(mDevice);

		// Pipeline Cache
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VK_CHECK_RESULT(vkCreatePipelineCache(mDevice->GetVulkanDevice(), &pipelineCacheCreateInfo, nullptr, &mPipelineCache));
	}
}