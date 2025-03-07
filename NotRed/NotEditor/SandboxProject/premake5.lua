
ProjectName = "Sandbox"
RootDirectory = "../../"

workspace "%{ProjectName}"
	architecture "x64"
	targetdir "build"
	startproject "%{ProjectName}"
	
	configurations 
	{ 
		"Debug", 
		"Release",
		"Dist"
	}

local OutputDir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "NotRed"
project "Not-ScriptCore"
	location "%{RootDirectory}Not-ScriptCore"
	kind "SharedLib"
	language "C#"

	targetdir ("%{RootDirectory}NotEditor/Resources/Scripts")
	objdir ("%{RootDirectory}NotEditor/Resources/Scripts/Intermediates")

	files 
	{
		"%{RootDirectory}Not-ScriptCore/Source/**.cs", 
	}

group ""
project "%{ProjectName}"
	location "Assets/Scripts"
	kind "SharedLib"
	language "C#"

	targetname "%{ProjectName}"
	targetdir ("%{prj.location}/Binaries")
	objdir ("%{RootDirectory}NotEditor/Resources/Scripts/Intermediates")

	files 
	{
		"Assets/Scripts/src/**.cs", 
	}

	links
	{
		"Not-ScriptCore"
	}