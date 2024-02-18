workspace "NotRed"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

outputDir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "NotRed"
	location "NotRed"
	kind "SharedLib"
	language "C++"

	targetdir ("bin/" .. outputDir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputDir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/src"
		"%{prj.name}/vendor/spdlog/include"
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
			("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputDir .. "/Sandbox")
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

	targetdir ("bin/" .. outputDir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputDir .. "/%{prj.name}")
	
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