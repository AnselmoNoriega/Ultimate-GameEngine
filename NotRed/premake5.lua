workspace "NotRed"
	architecture "x64"
	targetdir "build"

	configurations 
	{ 
		"Debug", 
        "Release",
        "Dist"
    }

local outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "NotRed"
    location "NotRed"
    kind "SharedLib"
    language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files 
	{ 
		"%{prj.name}/**.h", 
		"%{prj.name}/**.c", 
		"%{prj.name}/**.hpp", 
		"%{prj.name}/**.cpp" 
    }

    includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor",
	}

	filter "system:windows"
		cppdialect "C++17"
        staticruntime "On"

		defines 
		{ 
            "NR_PLATFORM_WINDOWS",
            "NOT_BUILD_BUILD_DLL",
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

    filter { "system:windows", "configurations:Release" }
        buildoptions "/MT"

project "Sandbox"
    location "Sandbox"
    kind "ConsoleApp"
    language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	dependson 
	{ 
		"NotRed"
    }

	files 
	{ 
		"%{prj.name}/**.h", 
		"%{prj.name}/**.c", 
		"%{prj.name}/**.hpp", 
		"%{prj.name}/**.cpp" 
	}

	includedirs 
	{
        "%{prj.name}/src",
        "NotRed/src",
        "NotRed/vendor",
    }

	filter "system:windows"
        cppdialect "C++17"
        staticruntime "On"

		links 
		{ 
			"NotRed"
		}

		defines 
		{ 
            "NR_PLATFORM_WINDOWS",
		}

        postbuildcommands
        {
            ("{COPY} ../bin/" .. outputdir .. "/NotRed/NotRed.dll %{cfg.targetdir}")
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

    filter { "system:windows", "configurations:Release" }
        buildoptions "/MT"    