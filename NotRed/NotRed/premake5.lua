project "NotRed"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "nrpch.h"
	pchsource "src/nrpch.cpp"

	files
	{
		"src/**.h",
		"src/**.cpp",
		"vendor/stb_image/**.h",
		"vendor/stb_image/**.cpp",
		"vendor/glm/glm/**.hpp",
		"vendor/glm/glm/**.inl",

		"vendor/ImGuizmo/ImGuizmo.h",
		"vendor/ImGuizmo/ImGuizmo.cpp"
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE"
	}

	includedirs
	{
		"src",
		"vendor/spdlog/include",
		"%{IncludeDir.Assimp}",
		"%{IncludeDir.Box2D}",
		"%{IncludeDir.Entt}",
		"%{IncludeDir.FileWatch}",
		"%{IncludeDir.Glm}",
		"%{IncludeDir.MsdfGen}",
		"%{IncludeDir.MsdfAtlasGen}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.Mono}",
		"%{IncludeDir.Stb}",
		"%{IncludeDir.Yaml}"
		--"%{IncludeDir.VulkanSDK}"
	}

	links
	{
		"Box2D",
		"GLFW",
		"Glad",
		"ImGui",
		"msdf-atlas-gen",
		"yaml-cpp",
		"opengl32.lib",

		"%{Library.Assimp}",
		"%{Library.Mono}"
		--"%{Library.Vulkan}"
	}

	postbuildcommands 
	{
        'copy /y "%{prj.location}vendor\\assimp\\bin\\*.dll" "%{cfg.targetdir}"'
    }

	filter "files:vendor/ImGuizmo/**.cpp"
	flags { "NoPCH" }

	filter "system:windows"
		systemversion "latest"

		defines
		{
		}

		links
		{
			"%{Library.WinSock}",
			"%{Library.WinMM}",
			"%{Library.WinVersion}",
			"%{Library.BCrypt}",
		}

	filter "configurations:Debug"
		defines "NR_DEBUG"
		runtime "Debug"
		symbols "on"

		--links
		--{
		--	"%{Library.ShaderC_Debug}",
		--	"%{Library.SPIRV_Cross_Debug}",
		--	"%{Library.SPIRV_Cross_GLSL_Debug}"
		--}

	filter "configurations:Release"
		defines "NR_RELEASE"
		runtime "Release"
		optimize "on"
		
		--links
		--{
		--	"%{Library.ShaderC_Release}",
		--	"%{Library.SPIRV_Cross_Release}",
		--	"%{Library.SPIRV_Cross_GLSL_Release}"
		--}

	filter "configurations:Dist"
		defines "NR_DIST"
		runtime "Release"
		optimize "on"
		
		--links
		--{
		--	"%{Library.ShaderC_Release}",
		--	"%{Library.SPIRV_Cross_Release}",
		--	"%{Library.SPIRV_Cross_GLSL_Release}"
		--}