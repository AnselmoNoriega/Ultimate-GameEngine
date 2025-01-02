workspace "NotRed"
	architecture "x64"
	targetdir "build"
	
	configurations 
	{ 
		"Debug", 
		"Release",
		"Dist"
	}

	flags
	{
		"MultiProcessorCompile"
	}

	startproject "NotEditor"
	
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "Dependencies.lua"

group "Dependencies"
include "NotRed/vendor/GLFW"
include "NotRed/vendor/Glad"
include "NotRed/vendor/ImGui"
include "NotRed/vendor/Box2D"
include "NotRed/vendor/Optick"
include "NotRed/vendor/ozz-animation"
group ""

group "Core"
project "NotRed"
	location "NotRed"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "nrpch.h"
	pchsource "NotRed/src/nrpch.cpp"

	files 
	{ 
		"%{prj.name}/src/**.h", 
		"%{prj.name}/src/**.c", 
		"%{prj.name}/src/**.hpp", 
		"%{prj.name}/src/**.cpp",

		"%{prj.name}/vendor/FastNoise/**.cpp",

		"%{prj.name}/vendor/yaml-cpp/src/**.cpp",
		"%{prj.name}/vendor/yaml-cpp/src/**.h",
		"%{prj.name}/vendor/yaml-cpp/include/**.h",
		"%{prj.name}/vendor/VulkanMemoryAllocator/**.h",
		"%{prj.name}/vendor/VulkanMemoryAllocator/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor",
		"%{prj.name}/..",
		"%{IncludeDir.Assimp}",
		"%{IncludeDir.Box2D}",
		"%{IncludeDir.FBX}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.Glm}",
		"%{IncludeDir.Yaml}",
		"%{IncludeDir.Stb}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.Entt}",
		"%{IncludeDir.Mono}",
		"%{IncludeDir.Optick}",
		"%{IncludeDir.Ozz}",
		"%{IncludeDir.PhysX}",
		"%{IncludeDir.FastNoise}",
		"%{IncludeDir.Vulkan}",
		"%{IncludeDir.NsightAftermath}",
		"%{IncludeDir.MiniAudio}"
	}
	
	links 
	{ 
		"Box2D",
		"GLFW",
		"Glad",
		"Optick",
		"ImGui",
		"opengl32.lib",
		"ozz_base",
		"ozz_animation",
		"ozz_geometry",
		"ozz_animation_offline",
		"ozz_animation_fbx",
		"%{Library.Vulkan}",
		"%{Library.Mono}",
		"%{Library.PhysX}",
		"%{Library.PhysXCharacterKinematic}",
		"%{Library.PhysXCommon}",
		"%{Library.PhysXCooking}",
		"%{Library.PhysXExtensions}",
		"%{Library.PhysXFoundation}",
		"%{Library.PhysXPvd}",
		"%{Library.NsightAftermath}"
	}

	defines
	{
		"PX_PHYSX_STATIC_LIB"
	}
	
	filter "files:NotRed/vendor/FastNoise/**.cpp or files:NotRed/vendor/yaml-cpp/src/**.cpp"
   	flags { "NoPCH" }

	filter "system:windows"
		systemversion "latest"
		
		defines 
		{ 
			"NR_PLATFORM_WINDOWS",
			"NR_BUILD_DLL"
		}

	filter "configurations:Debug"
		defines "NR_DEBUG"
		symbols "on"

		links
		{
			"%{Library.FBX_Debug}",
			"%{Library.FBX_XML2_Debug}",
			"%{Library.FBX_ZLIB_Debug}", 

			"%{Library.ShaderC_Debug}",
			"%{Library.SPIRV_Cross_Debug}",
			"%{Library.SPIRV_Cross_GLSL_Debug}"
		}
				
	filter "configurations:Release"
		defines
		{
			"NR_RELEASE",
			"NDEBUG"
		}
		optimize "on"
		
		links
		{
			"%{Library.FBX_Release}",
			"%{Library.FBX_XML2_Release}",
			"%{Library.FBX_ZLIB_Release}", 

			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}"
		}

	filter "configurations:Dist"
		optimize "on"
		
		defines
		{
			"NR_DIST",
			"NDEBUG"
		}
		
		links
		{
			"%{Library.FBX_Release}",
			"%{Library.FBX_XML2_Release}",
			"%{Library.FBX_ZLIB_Release}", 

			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}"
		}

project "Not-ScriptCore"
	location "Not-ScriptCore"
	kind "SharedLib"
	language "C#"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files 
	{
		"%{prj.name}/Source/**.cs", 
	}
group ""

group "Tools"
project "NotEditor"
	location "NotEditor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	links 
	{ 
		"NotRed",
		"ozz_animation_offline",
		"ozz_animation_fbx"
	}
	
	files 
	{ 
		"%{prj.name}/src/**.h", 
		"%{prj.name}/src/**.c", 
		"%{prj.name}/src/**.hpp", 
		"%{prj.name}/src/**.cpp" 
	}
	
	includedirs 
	{
		"%{prj.name}/src",
		"NotRed/src",
		"NotRed/vendor",
		"%{IncludeDir.Entt}",
		"%{IncludeDir.FBX}",
		"%{IncludeDir.Glm}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.Vulkan}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.MiniAudio}",
		"%{IncludeDir.Ozz}"
	}

	postbuildcommands 
	{
		'{COPY} "../NotRed/vendor/NsightAftermath/lib/GFSDK_Aftermath_Lib.x64.dll" "%{cfg.targetdir}"',
		'{COPY} "../NotRed/vendor/PhysX/win64/PhysX_64.dll" "%{cfg.targetdir}"',
		'{COPY} "../NotRed/vendor/PhysX/win64/PhysXCommon_64.dll" "%{cfg.targetdir}"',
		'{COPY} "../NotRed/vendor/PhysX/win64/PhysXCooking_64.dll" "%{cfg.targetdir}"',
		'{COPY} "../NotRed/vendor/PhysX/win64/PhysXFoundation_64.dll" "%{cfg.targetdir}"'
	}
	
	filter "system:windows"
		systemversion "latest"
				
		defines 
		{ 
			"NR_PLATFORM_WINDOWS"
		}
	
	filter "configurations:Debug"
		defines "NR_DEBUG"
		symbols "on"

		links
		{
			"NotRed/vendor/assimp/bin/Debug/assimp-vc143-mtd.lib",
			"%{Library.FBX_Debug}",
			"%{Library.FBX_XML2_Debug}",
			"%{Library.FBX_ZLIB_Debug}"
		}

		postbuildcommands 
		{
			'{COPY} "../NotRed/vendor/assimp/bin/Debug/assimp-vc143-mtd.dll" "%{cfg.targetdir}"',
			'{COPY} "../NotRed/vendor/mono/bin/Debug/mono-2.0-sgen.dll" "%{cfg.targetdir}"',
		    '{COPY} "../NotRed/vendor/Vulkan/win64/shaderc_sharedd.dll" "%{cfg.targetdir}"'
		}
				
	filter "configurations:Release"
		defines
		{
			"NR_RELEASE",
			"NDEBUG"
		}
		optimize "on"

		links
		{
			"NotRed/vendor/assimp/bin/Release/assimp-vc143-mt.lib",
			"%{Library.FBX_Release}",
			"%{Library.FBX_XML2_Release}",
			"%{Library.FBX_ZLIB_Release}"
		}

		postbuildcommands 
		{
			'{COPY} "../NotRed/vendor/assimp/bin/Release/assimp-vc143-mt.dll" "%{cfg.targetdir}"',
			'{COPY} "../NotRed/vendor/mono/bin/Debug/mono-2.0-sgen.dll" "%{cfg.targetdir}"'
		}

	filter "configurations:Dist"
		defines "NR_DIST"
		optimize "on"

		links
		{
			"NotRed/vendor/assimp/bin/Release/assimp-vc143-mt.lib",
			"%{Library.FBX_Release}",
			"%{Library.FBX_XML2_Release}",
			"%{Library.FBX_ZLIB_Release}"
		}

		postbuildcommands 
		{
			'{COPY} "../NotRed/vendor/assimp/bin/Release/assimp-vc143-mtd.dll" "%{cfg.targetdir}"',
			'{COPY} "../NotRed/vendor/mono/bin/Debug/mono-2.0-sgen.dll" "%{cfg.targetdir}"'
		}
group ""

group "Runtime"
project "Not-Runtime"
	location "Not-Runtime"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	links 
	{ 
		"NotRed"
	}
	
	files 
	{ 
		"%{prj.name}/src/**.h", 
		"%{prj.name}/src/**.c", 
		"%{prj.name}/src/**.hpp", 
		"%{prj.name}/src/**.cpp" 
	}
	
	includedirs 
	{
		"%{prj.name}/src",
		"NotRed/src",
		"NotRed/vendor",
		"%{IncludeDir.entt}",
		"%{IncludeDir.FBX}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.Vulkan}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.Ozz}"
	}
	postbuildcommands 
	{
		'{COPY} "../NotRed/vendor/NsightAftermath/lib/GFSDK_Aftermath_Lib.x64.dll" "%{cfg.targetdir}"'
	}
	
	filter "system:windows"
		systemversion "latest"
				
		defines 
		{ 
			"NR_PLATFORM_WINDOWS"
		}
	
	filter "configurations:Debug"
		defines "NR_DEBUG"
		symbols "on"
		links
		{
			"NotRed/vendor/assimp/bin/Debug/assimp-vc143-mtd.lib"
		}
		postbuildcommands 
		{
			'{COPY} "../NotRed/vendor/assimp/bin/Debug/assimp-vc143-mtd.dll" "%{cfg.targetdir}"',
			'{COPY} "../NotRed/vendor/mono/bin/Debug/mono-2.0-sgen.dll" "%{cfg.targetdir}"',
			'{COPY} "../NotRed/vendor/Vulkan/win64/shaderc_sharedd.dll" "%{cfg.targetdir}"'
		}
				
	filter "configurations:Release"
		defines "NR_RELEASE"
		optimize "on"
		links
		{
			"NotRed/vendor/assimp/bin/Release/assimp-vc143-mt.lib"
		}
		postbuildcommands 
		{
			'{COPY} "../NotRed/vendor/assimp/bin/Release/assimp-vc143-mt.dll" "%{cfg.targetdir}"',
			'{COPY} "../NotRed/vendor/mono/bin/Debug/mono-2.0-sgen.dll" "%{cfg.targetdir}"'
		}
	filter "configurations:Dist"
		defines "NR_DIST"
		optimize "on"
		links
		{
			"NotRed/vendor/assimp/bin/Release/assimp-vc143-mt.lib"
		}
		postbuildcommands 
		{
			'{COPY} "../NotRed/vendor/assimp/bin/Release/assimp-vc143-mtd.dll" "%{cfg.targetdir}"',
			'{COPY} "../NotRed/vendor/mono/bin/Debug/mono-2.0-sgen.dll" "%{cfg.targetdir}"'
		}
group ""