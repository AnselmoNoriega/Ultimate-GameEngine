include "./vendor/premake/premake_customization/solution_items.lua"
include "Dependencies.lua"

workspace "NotRed"
	architecture "x86_64"
	startproject "NotEditor"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	solution_items
	{
		".editorconfig"
	}

	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
	include "NotRed/vendor/GLFW"
	include "NotRed/vendor/glad"
	include "NotRed/vendor/imgui"
	include "NotRed/vendor/yaml-cpp"

group ""

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
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/stb_image/**.h",
		"%{prj.name}/vendor/stb_image/**.cpp",

		"%{prj.name}/vendor/ImGuizmo/ImGuizmo.h",
		"%{prj.name}/vendor/ImGuizmo/ImGuizmo.cpp"
	}


	defines
	{
		"_CRT_SECURE_NO_WARNINGS"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.Glm}",
		"%{IncludeDir.Stb}",
		"%{IncludeDir.Entt}",
		"%{IncludeDir.Yaml}",
		"%{IncludeDir.ImGuizmo}"
	}

	links
	{
		"GLFW",
		"glad",
		"imgui",
		"yaml-cpp",
		"opengl32.lib"
	}

	filter "files:NotRed/vendor/ImGuizmo/**.cpp"
	flags { "NoPCH" }

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"NR_PLATFORM_WINDOWS",
			"NR_BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

	filter "configurations:Debug"
		defines "NR_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "NR_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "NR_DIST"
		runtime "Release"
		optimize "on"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "on"

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
		"%{IncludeDir.Glm}",

		"%{IncludeDir.Entt}"
	}

	links
	{
		"NotRed"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"NR_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "NR_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "NR_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "NR_DIST"
		runtime "Release"
		optimize "on"

project "NotEditor"
	location "NotEditor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "on"

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
		"%{IncludeDir.Glm}",
		"%{IncludeDir.Entt}",
		"%{IncludeDir.ImGuizmo}"
	}

	links
	{
		"NotRed"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"NR_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "NR_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "NR_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "NR_DIST"
		runtime "Release"
		optimize "on"