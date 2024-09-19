
-- NotRed Dependencies

IncludeDir = {}
IncludeDir["Assimp"] = "NotRed/vendor/assimp/include"
IncludeDir["Box2D"] = "NotRed/vendor/box2D/include"
IncludeDir["Entt"] = "NotRed/vendor/Entt/include"
IncludeDir["FastNoise"] = "NotRed/vendor/FastNoise"
IncludeDir["Glad"] = "NotRed/vendor/glad/include"
IncludeDir["GLFW"] = "NotRed/vendor/GLFW/include"
IncludeDir["Glm"] = "NotRed/vendor/glm"
IncludeDir["ImGui"] = "NotRed/vendor/imgui"
IncludeDir["Mono"] = "NotRed/vendor/mono/include"
IncludeDir["Stb"] = "NotRed/vendor/stb_image"
IncludeDir["Yaml"] = "NotRed/vendor/yaml-cpp/include"
IncludeDir["PhysX"] = "NotRed/vendor/PhysX/include"
IncludeDir["Vulkan"] = "NotRed/vendor/Vulkan/Include"

LibraryDir = {}
LibraryDir["PhysX"] = "%{wks.location}/NotRed/vendor/PhysX/lib/%{cfg.buildcfg}"
LibraryDir["VulkanSDK"] = "%{wks.location}/NotRed/vendor/Vulkan/lib"

Library = {}
Library["Mono"] = "vendor/mono/lib/Debug/mono-2.0-sgen.lib"
Library["PhysX"] = "vendor/PhysX/lib/%{cfg.buildcfg}/PhysX_64.lib"
Library["PhysXCharacterKinematic"] = "vendor/PhysX/lib/%{cfg.buildcfg}/PhysXCharacterKinematic_static_64.lib"
Library["PhysXCommon"] = "vendor/PhysX/lib/%{cfg.buildcfg}/PhysXCommon_64.lib"
Library["PhysXCooking"] = "vendor/PhysX/lib/%{cfg.buildcfg}/PhysXCooking_64.lib"
Library["PhysXExtensions"] = "vendor/PhysX/lib/%{cfg.buildcfg}/PhysXExtensions_static_64.lib"
Library["PhysXFoundation"] = "vendor/PhysX/lib/%{cfg.buildcfg}/PhysXFoundation_64.lib"
Library["PhysXPvd"] = "vendor/PhysX/lib/%{cfg.buildcfg}/PhysXPvdSDK_static_64.lib"

----------Vulkan--------------
Library ["Vulkan"] = "vendor/Vulkan/lib/vulkan-1.lib"

Library["ShaderC_Debug"] = "vendor/Vulkan/lib/shaderc_sharedd.lib"
Library["SPIRV_Cross_Debug"] = "vendor/Vulkan/lib/spirv-cross-cored.lib"
Library["SPIRV_Cross_GLSL_Debug"] = "vendor/Vulkan/lib/spirv-cross-glsld.lib"
Library["SPIRV_Tools_Debug"] = "vendor/Vulkan/lib/SPIRV-Toolsd.lib"

Library["ShaderC_Release"] = "vendor/Vulkan/lib/shaderc_shared.lib"
Library["SPIRV_Cross_Release"] = "vendor/Vulkan/lib/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL_Release"] = "vendor/Vulkan/lib/spirv-cross-glsl.lib"
