workspace "NotRed"
	architecture "x64"
	targetdir "build"
	
	configurations 
	{ 
		"Debug", 
        "Release",
        "Dist"
    }
    
	startproject "Sandbox"
    
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "NotRed/vendor/GLFW/include"
IncludeDir["Glad"] = "NotRed/vendor/Glad/include"
IncludeDir["ImGui"] = "NotRed/vendor/ImGui"
IncludeDir["GLM"] = "NotRed/vendor/glm"

include "NotRed/vendor/GLFW"
include "NotRed/vendor/Glad"
include "NotRed/vendor/ImGui"

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
		"%{prj.name}/src/**.cpp" 
    }

    includedirs
	{
		"%{prj.name}/src",
        "%{prj.name}/vendor",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.Glad}",
        "%{IncludeDir.GLM}",
        "%{IncludeDir.ImGui}"
    }
    
    links 
	{ 
        "GLFW",
        "Glad",
        "ImGui",
		"opengl32.lib"
    }
    
	filter "system:windows"
        systemversion "latest"
        
		defines 
		{ 
            "NR_PLATFORM_WINDOWS",
            "NR_BUILD_DLL"
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
        "%{IncludeDir.GLM}"
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
                
    filter "configurations:Release"
        defines "NR_RELEASE"
        optimize "on"

    filter "configurations:Dist"
        defines "NR_DIST"
        optimize "on"