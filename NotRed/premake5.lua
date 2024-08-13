workspace "NotRed"
	architecture "x64"
	targetdir "build"

	configurations 
	{ 
		"Debug", 
        "Release",
        "Dist"
    }


    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

    IncludeDir = {}
    IncludeDir["GLFW"] = "NotRed/vendor/GLFW/include"
    IncludeDir["Glad"] = "NotRed/vendor/Glad/include"
    IncludeDir["ImGui"] = "NotRed/vendor/ImGui"
    
    include "NotRed/vendor/GLFW"
    include "NotRed/vendor/Glad"
    include "NotRed/vendor/ImGui"

project "NotRed"
    location "NotRed"
    kind "SharedLib"
    language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

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
        "%{prj.name}/vendor",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.Glad}",
        "%{IncludeDir.ImGui}"
	}

    links 
	{ 
        "GLFW",
        "Glad",
        "ImGui",
    }

	filter "system:windows"
        systemversion "latest"
		cppdialect "C++17"
        staticruntime "On"

		defines 
		{ 
            "NR_PLATFORM_WINDOWS",
            "NOT_BUILD_BUILD_DLL",
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