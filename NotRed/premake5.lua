workspace "NotRed"
	architecture "x64"
	startproject "Sandbox"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "NotRed/vendor/GLFW/include"
IncludeDir["Glad"] = "NotRed/vendor/glad/include"
IncludeDir["ImGui"] = "NotRed/vendor/imgui"
IncludeDir["Glm"] = "NotRed/vendor/glm"

group "Dependencies"
	include "NotRed/vendor/GLFW"
	include "NotRed/vendor/glad"
	include "NotRed/vendor/imgui"

group ""

project "NotRed"
	location "NotRed"
	kind "SharedLib"
	language "C++"
	staticruntime "off"

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
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.Glm}"
	}

	links
	{
		"GLFW",
		"glad",
		"imgui",
		"opengl32.lib"
	}

	filter "system:windows"
		cppdialect "C++20"
		systemversion "latest"

		defines
		{
			"NR_PLATFORM_WINDOWS",
			"NR_BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} \"../bin/" .. outputdir .. "/Sandbox/\"")
		}

	filter "configurations:Debug"
		defines "NR_DEBUG"
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		defines "NR_RELEASE"
		runtime "Release"
		optimize "On"

	filter "configurations:Dist"
		defines "NR_DIST"
		runtime "Release"
		optimize "On"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	staticruntime "off"

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
		"NotRed/src",
		"NotRed/vendor",
		"%{IncludeDir.Glm}"
	}

	links
	{
		"NotRed"
	}

	filter "system:windows"
		cppdialect "C++20"
		systemversion "latest"

		defines
		{
			"NR_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "NR_DEBUG"
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		defines "NR_RELEASE"
		runtime "Release"
		optimize "On"

	filter "configurations:Dist"
		defines "NR_DIST"
		runtime "Release"
		optimize "On"