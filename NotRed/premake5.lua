workspace "NotRed"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "NotRed/vendor/GLFW/include"

include "NotRed/vendor/GLFW"

project "NotRed"
	location "NotRed"
	kind "SharedLib"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "nrpch.h"
	pchsource "NotRed/src/nrpch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}"
	}

	links
	{
		"GLFW",
		"opengl32.lib"
	}

	filter "system:windows"
		cppdialect "C++20"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"NR_PLATFORM_WINDOWS",
			"NR_BUILD_DLL"
		}

		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Sandbox")
		}

	filter "configurations:Debug"
		defines "NR_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "NR_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "NR_DIST"
		optimize "On"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	
	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"NotRed/vendor/spdlog/include",
		"NotRed/src"
	}

	links
	{
		"NotRed"
	}

	filter "system:windows"
		cppdialect "C++20"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"NR_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "NR_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "NR_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "NR_DIST"
		optimize "On"