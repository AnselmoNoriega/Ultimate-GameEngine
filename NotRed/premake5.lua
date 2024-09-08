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

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["Box2D"] = "NotRed/vendor/box2D/include"
IncludeDir["Entt"] = "NotRed/vendor/Entt/include"
IncludeDir["FastNoise"] = "NotRed/vendor/FastNoise"
IncludeDir["Glad"] = "NotRed/vendor/glad/include"
IncludeDir["GLFW"] = "NotRed/vendor/GLFW/include"
IncludeDir["Glm"] = "NotRed/vendor/glm"
IncludeDir["ImGui"] = "NotRed/vendor/imgui"
IncludeDir["Mono"] = "NotRed/vendor/mono/include"
IncludeDir["PhysX"] = "NotRed/vendor/PhysX/include"

LibraryDir = {}
LibraryDir["Mono"] = "vendor/mono/lib/Debug/mono-2.0-sgen.lib"
LibraryDir["PhysX"] = "vendor/PhysX/lib/%{cfg.buildcfg}/PhysX_64.lib"
LibraryDir["PhysXCharacterKinematic"] = "vendor/PhysX/lib/%{cfg.buildcfg}/PhysXCharacterKinematic_static_64.lib"
LibraryDir["PhysXCommon"] = "vendor/PhysX/lib/%{cfg.buildcfg}/PhysXCommon_64.lib"
LibraryDir["PhysXCooking"] = "vendor/PhysX/lib/%{cfg.buildcfg}/PhysXCooking_64.lib"
LibraryDir["PhysXExtensions"] = "vendor/PhysX/lib/%{cfg.buildcfg}/PhysXExtensions_static_64.lib"
LibraryDir["PhysXFoundation"] = "vendor/PhysX/lib/%{cfg.buildcfg}/PhysXFoundation_64.lib"
LibraryDir["PhysXPvd"] = "vendor/PhysX/lib/%{cfg.buildcfg}/PhysXPvdSDK_static_64.lib"

group "Dependencies"
include "NotRed/vendor/GLFW"
include "NotRed/vendor/Glad"
include "NotRed/vendor/ImGui"
include "NotRed/vendor/Box2D"
group ""

group "Core"
project "NotRed"
	location "NotRed"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	staticruntime "on"

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
		"%{prj.name}/vendor/yaml-cpp/include/**.h"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor",
		"%{IncludeDir.Box2D}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.Glm}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.Entt}",
		"%{IncludeDir.Mono}",
		"%{IncludeDir.PhysX}",
		"%{IncludeDir.FastNoise}",
		"%{prj.name}/vendor/assimp/include",
		"%{prj.name}/vendor/stb_image",
		"%{prj.name}/vendor/yaml-cpp/include"
	}
	
	links 
	{ 
		"Box2D",
		"GLFW",
		"Glad",
		"ImGui",
		"opengl32.lib",
		"%{LibraryDir.Mono}",
		"%{LibraryDir.PhysX}",
		"%{LibraryDir.PhysXCharacterKinematic}",
		"%{LibraryDir.PhysXCommon}",
		"%{LibraryDir.PhysXCooking}",
		"%{LibraryDir.PhysXExtensions}",
		"%{LibraryDir.PhysXFoundation}",
		"%{LibraryDir.PhysXPvd}"
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
				
	filter "configurations:Release"
		defines "NR_RELEASE"
		optimize "on"

	filter "configurations:Dist"
		defines "NR_DIST"
		optimize "on"

project "NotRed-ScriptCore"
	location "NotRed-ScriptCore"
	kind "SharedLib"
	language "C#"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files 
	{
		"%{prj.name}/src/**.cs", 
	}
group ""

group "Tools"
project "NotEditor"
	location "NotEditor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "on"
	
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
		"%{IncludeDir.Entt}",
		"%{IncludeDir.Glm}"
	}

	postbuildcommands 
	{
		'{COPY} "../NotEditor/Assets" "%{cfg.targetdir}/Assets"'
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
			"NotRed/vendor/assimp/bin/Debug/assimp-vc141-mtd.lib"
		}

		postbuildcommands 
		{
			'{COPY} "../NotRed/vendor/assimp/bin/Debug/assimp-vc141-mtd.dll" "%{cfg.targetdir}"',
			'{COPY} "../NotRed/vendor/mono/bin/Debug/mono-2.0-sgen.dll" "%{cfg.targetdir}"'
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
			"NotRed/vendor/assimp/bin/Release/assimp-vc141-mt.lib"
		}

		postbuildcommands 
		{
			'{COPY} "../NotRed/vendor/assimp/bin/Release/assimp-vc141-mt.dll" "%{cfg.targetdir}"',
			'{COPY} "../NotRed/vendor/mono/bin/Debug/mono-2.0-sgen.dll" "%{cfg.targetdir}"'
		}

	filter "configurations:Dist"
		defines "NR_DIST"
		optimize "on"

		links
		{
			"NotRed/vendor/assimp/bin/Release/assimp-vc141-mt.lib"
		}

		postbuildcommands 
		{
			'{COPY} "../NotRed/vendor/assimp/bin/Release/assimp-vc141-mtd.dll" "%{cfg.targetdir}"',
			'{COPY} "../NotRed/vendor/mono/bin/Debug/mono-2.0-sgen.dll" "%{cfg.targetdir}"'
		}
group ""

group "External"
workspace "Sandbox"
	architecture "x64"
	targetdir "build"

	configurations 
	{ 
		"Debug", 
		"Release",
		"Dist"
	}

project "ExampleApp"
	location "ExampleApp"
	kind "SharedLib"
	language "C#"

	targetdir ("NotEditor/Assets/Scripts")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files 
	{
		"%{prj.name}/src/**.cs", 
	}

	links
	{
		"NotRed-ScriptCore"
	}
group ""