project "Assimp"
    kind "StaticLib"
    language "C++"
	staticruntime "off"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files 
	{
        "include/assimp/**.h",
        "include/assimp/**.hpp",
        "code/**.cpp",
        "code/**.h",
        "code/**.c"
    }

	includedirs
	{
		"include",
		"src"
	}

    filter "configurations:Debug"
		runtime "Debug"
        defines { "ASSIMP_BUILD_DLL" }
        symbols "On"

    filter "configurations:Release"
		runtime "Release"
        defines { "ASSIMP_BUILD_DLL" }
        optimize "On"

    filter "configurations:Dist"
		runtime "Release"
        defines { "ASSIMP_BUILD_DLL" }
        optimize "On"
		symbols "off"