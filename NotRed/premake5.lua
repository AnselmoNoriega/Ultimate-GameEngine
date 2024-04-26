include "./vendor/premake/premake_customization/solution_items.lua"
include "Dependencies.lua"

workspace "NotRed"
	architecture "x86_64"
	startproject "NotEditor"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	solution_items
	{
		".editorconfig"
	}

	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
	include "vendor/premake"
	include "NotRed/vendor/Box2D"
	include "NotRed/vendor/GLFW"
	include "NotRed/vendor/glad"
	include "NotRed/vendor/imgui"
	include "NotRed/vendor/msdf-atlas-gen"
	include "NotRed/vendor/yaml-cpp"
group ""

group "Core"
	include "NotRed"
	include "NotRed-ScriptCore"
group ""

group "Tools"
	include "NotEditor"
group ""

group "Misc"
	include "Sandbox"
group ""