ProjectName = "$PROJECT_NAME$"
NotRedRootDirectory = os.getenv("NOTRED_DIR")

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

group "NotRed"
project "Not-ScriptCore"
	location "%{NotRedRootDirectory}/Not-ScriptCore"
	kind "SharedLib"
	language "C#"
	targetdir ("%{NotRedRootDirectory}/NotEditor/Resources/Scripts")
	objdir ("%{NotRedRootDirectory}/NotEditor/Resources/Scripts/Intermediates")

	files
	{
		"%{NotRedRootDirectory}/Not-ScriptCore/Source/**.cs"
	}

group ""

project "%{ProjectName}"
	location "Assets/Scripts"
	kind "SharedLib"
	language "C#"
	targetname "%{ProjectName}"
	targetdir ("%{prj.location}/Binaries")
	objdir ("%{NotRedRootDirectory}/NotEditor/Resources/Scripts/Intermediates")

	files 
	{
		"Assets/Scripts/Source/**.cs", 
	}
    
	links
	{
		"Not-ScriptCore"
	}