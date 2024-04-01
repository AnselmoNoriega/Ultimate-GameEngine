
-- NotRed Dependencies

VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["GLFW"] = "%{wks.location}/NotRed/vendor/GLFW/include"
IncludeDir["Glad"] = "%{wks.location}/NotRed/vendor/Glad/include"
IncludeDir["Glm"] = "%{wks.location}/NotRed/vendor/glm"
IncludeDir["Stb"] = "%{wks.location}/NotRed/vendor/stb_image"
IncludeDir["ImGui"] = "%{wks.location}/NotRed/vendor/ImGui"
IncludeDir["ImGuizmo"] = "%{wks.location}/NotRed/vendor/ImGuizmo"
IncludeDir["Yaml"] = "%{wks.location}/NotRed/vendor/yaml-cpp/include"
IncludeDir["Entt"] = "%{wks.location}/NotRed/vendor/entt/include"
IncludeDir["Shaderc"] = "%{wks.location}/NotRed/vendor/shaderc/include"
IncludeDir["SPIRV_Cross"] = "%{wks.location}/NotRed/vendor/SPIRV-Cross"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"

LibraryDir = {}

LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"
LibraryDir["VulkanSDK_Debug"] = "%{wks.location}/NotRed/vendor/VulkanSDK/Lib"

Library = {}
Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
Library["VulkanUtils"] = "%{LibraryDir.VulkanSDK}/VkLayer_utils.lib"

Library["ShaderC_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/shaderc_sharedd.lib"
Library["SPIRV_Cross_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/spirv-cross-cored.lib"
Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/spirv-cross-glsld.lib"
Library["SPIRV_Tools_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/SPIRV-Toolsd.lib"

Library["ShaderC_Release"] = "%{LibraryDir.VulkanSDK}/shaderc_shared.lib"
Library["SPIRV_Cross_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsl.lib"
