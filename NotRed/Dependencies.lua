-- NotRed Dependencies

IncludeDir = {}
IncludeDir["Assimp"] = "%{wks.location}/NotRed/vendor/assimp/include"
IncludeDir["Box2D"] = "%{wks.location}/NotRed/vendor/box2D/include"
IncludeDir["Choc"] = "%{wks.location}/NotRed/vendor/choc"
IncludeDir["Entt"] = "%{wks.location}/NotRed/vendor/Entt/include"
IncludeDir["FastNoise"] = "%{wks.location}/NotRed/vendor/FastNoise"
IncludeDir["Farbot"] = "%{wks.location}/NotRed/vendor/farbot/include"
IncludeDir["Glad"] = "%{wks.location}/NotRed/vendor/glad/include"
IncludeDir["GLFW"] = "%{wks.location}/NotRed/vendor/GLFW/include"
IncludeDir["Glm"] = "%{wks.location}/NotRed/vendor/glm"
IncludeDir["ImGui"] = "%{wks.location}/NotRed/vendor/imgui"
IncludeDir["ImGuiNodeEditor"] = "%{wks.location}/NotRed/vendor/imgui-node-editor"
IncludeDir["Mono"] = "%{wks.location}/NotRed/vendor/mono/include"
IncludeDir["Stb"] = "%{wks.location}/NotRed/vendor/stb_image"
IncludeDir["Yaml"] = "%{wks.location}/NotRed/vendor/yaml-cpp/include"
IncludeDir["PhysX"] = "%{wks.location}/NotRed/vendor/PhysX/include"
IncludeDir["Vulkan"] = "%{wks.location}/NotRed/vendor/Vulkan/Include"
IncludeDir["NsightAftermath"] = "%{wks.location}/NotRed/vendor/NsightAftermath/include"
IncludeDir["MiniAudio"] = "%{wks.location}/NotRed/vendor/MiniAudio/include"
IncludeDir["Optick"] = "%{wks.location}/NotRed/vendor/Optick/src"
IncludeDir["Ozz"] = "%{wks.location}/NotRed/vendor/ozz-animation/include"
IncludeDir["Nlohmann"] = "%{wks.location}/NotRed/vendor/nlohmann-json"
IncludeDir["Msdf_atlas_gen"] = "%{wks.location}/NotRed/vendor/msdf-atlas-gen/msdf-atlas-gen"
IncludeDir["Msdfgen"] = "%{wks.location}/NotRed/vendor/msdf-atlas-gen/msdfgen"
IncludeDir["SOUL"] = "%{wks.location}/NotRed/vendor/SOUL/include"
IncludeDir["SOUL_CORE"] = "%{wks.location}/NotRed/vendor/SOUL/source/modules/soul_core"

LibraryDir = {}
LibraryDir["PhysX"] = "%{wks.location}/NotRed/vendor/PhysX/lib/%{cfg.buildcfg}"
LibraryDir["Vulkan"] = "%{wks.location}/NotRed/vendor/Vulkan/lib"
LibraryDir["NsightAftermath"] = "%{wks.location}/NotRed/vendor/NsightAftermath/lib"

Library = {}
Library["Mono"] = "vendor/mono/lib/Release/mono-2.0-sgen.lib"

Library["PhysX"] = "%{LibraryDir.PhysX}/PhysX_64.lib"
Library["PhysXCharacterKinematic"] = "%{LibraryDir.PhysX}/PhysXCharacterKinematic_static_64.lib"
Library["PhysXCommon"] = "%{LibraryDir.PhysX}/PhysXCommon_64.lib"
Library["PhysXCooking"] = "%{LibraryDir.PhysX}/PhysXCooking_64.lib"
Library["PhysXExtensions"] = "%{LibraryDir.PhysX}/PhysXExtensions_static_64.lib"
Library["PhysXFoundation"] = "%{LibraryDir.PhysX}/PhysXFoundation_64.lib"
Library["PhysXPvd"] = "%{LibraryDir.PhysX}/PhysXPvdSDK_static_64.lib"

Library["Vulkan"] = "%{LibraryDir.Vulkan}/vulkan-1.lib"

Library["ShaderC_Debug"] = "%{LibraryDir.Vulkan}/shaderc_sharedd.lib"
Library["SPIRV_Cross_Debug"] = "%{LibraryDir.Vulkan}/spirv-cross-cored.lib"
Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.Vulkan}/spirv-cross-glsld.lib"
Library["SPIRV_Tools_Debug"] = "%{LibraryDir.Vulkan}/SPIRV-Toolsd.lib"

Library["ShaderC_Release"] = "%{LibraryDir.Vulkan}/shaderc_shared.lib"
Library["SPIRV_Cross_Release"] = "%{LibraryDir.Vulkan}/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.Vulkan}/spirv-cross-glsl.lib"

Library["NsightAftermath"] = "%{LibraryDir.NsightAftermath}/GFSDK_Aftermath_Lib.x64.lib"
