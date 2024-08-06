project "Assimp"
    kind "StaticLib"
    language "C++"
	cppdialect "C++20"
	staticruntime "off"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files 
	{
        "include/assimp/**.h",
        "include/assimp/**.hpp",
        "code/**.h", 
        "code/**.cpp", 
        "code/**.hpp",
        "contrib/**.cpp",
        "contrib/**.h"
        
    }

	includedirs
	{
		"include",
		"src",
		"code",
        "contrib/zlib"
	}

    filter "configurations:Debug"
		runtime "Debug"
        defines { "ASSIMP_BUILD_DLL", "DEBUG" }
        symbols "On"

    filter "configurations:Release"
		runtime "Release"
        defines { "ASSIMP_BUILD_DLL", "NDEBUG" }
        optimize "On"

    filter "configurations:Dist"
		runtime "Release"
        defines { "ASSIMP_BUILD_DLL", "NDEBUG" }
        optimize "On"
		symbols "off"