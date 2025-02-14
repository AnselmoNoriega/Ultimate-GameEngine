project "SOUL"
	kind "StaticLib"
	language "C++"
    staticruntime "off"
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	files
	{
		"include/soul/soul_patch.h",
		"source/modules/soul_core/soul_core.cpp",
		"source/modules/soul_core/soul_core.h",
	}

	includedirs
	{
        "include",
		"source/modules/soul_core"
    }

	buildoptions { "/bigobj" }
	filter "system:windows"
		systemversion "latest"
		cppdialect "C++17"
		defines 
		{ 
			"WIN32"
		}

	filter "system:linux"
		pic "On"
		systemversion "latest"
		cppdialect "C++17"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
        
	filter "configurations:Release"
		runtime "Release"
		optimize "on"