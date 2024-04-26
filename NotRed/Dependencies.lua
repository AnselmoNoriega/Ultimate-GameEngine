
-- NotRed Dependencies

--VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["Box2D"] = "%{wks.location}/NotRed/vendor/Box2D/include"
IncludeDir["Entt"] = "%{wks.location}/NotRed/vendor/entt/include"
IncludeDir["FileWatch"] = "%{wks.location}/NotRed/vendor/filewatch"
IncludeDir["GLFW"] = "%{wks.location}/NotRed/vendor/GLFW/include"
IncludeDir["Glad"] = "%{wks.location}/NotRed/vendor/Glad/include"
IncludeDir["Glm"] = "%{wks.location}/NotRed/vendor/glm"
IncludeDir["ImGui"] = "%{wks.location}/NotRed/vendor/ImGui"
IncludeDir["ImGuizmo"] = "%{wks.location}/NotRed/vendor/ImGuizmo"
IncludeDir["Mono"] = "%{wks.location}/NotRed/vendor/mono/include"
IncludeDir["Stb"] = "%{wks.location}/NotRed/vendor/stb_image"
IncludeDir["Yaml"] = "%{wks.location}/NotRed/vendor/yaml-cpp/include"
IncludeDir["MsdfGen"] = "%{wks.location}/NotRed/vendor/msdf-atlas-gen/msdfgen"
IncludeDir["MsdfAtlasGen"] = "%{wks.location}/NotRed/vendor/msdf-atlas-gen/msdf-atlas-gen"
--IncludeDir["Shaderc"] = "%{wks.location}/NotRed/vendor/shaderc/include"
--IncludeDir["SPIRV_Cross"] = "%{wks.location}/NotRed/vendor/SPIRV-Cross"
--IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"

LibraryDir = {}

LibraryDir["Mono"] = "%{wks.location}/NotRed/vendor/mono/lib/%{cfg.buildcfg}"

Library = {}
Library["Mono"] = "%{LibraryDir.Mono}/libmono-static-sgen.lib"

--Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
--Library["VulkanUtils"] = "%{LibraryDir.VulkanSDK}/VkLayer_utils.lib"

--Library["ShaderC_Debug"] = "%{LibraryDir.VulkanSDK}/shaderc_sharedd.lib"
--Library["SPIRV_Cross_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-cored.lib"
--Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsld.lib"
--Library["SPIRV_Tools_Debug"] = "%{LibraryDir.VulkanSDK}/SPIRV-Toolsd.lib"
--Library["ShaderC_Release"] = "%{LibraryDir.VulkanSDK}/shaderc_shared.lib"
--Library["SPIRV_Cross_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-core.lib"
--Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsl.lib"

Library["WinSock"] = "Ws2_32.lib"
Library["WinMM"] = "Winmm.lib"
Library["WinVersion"] = "Version.lib"
Library["BCrypt"] = "Bcrypt.lib"
