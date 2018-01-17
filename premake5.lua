workspace "nova-ecs"
	configurations { "debug", "profiler", "release" }
	platforms      { "x64", "x32" }
	startproject   "test"
  	characterset   "MBCS"

	location( build_root )

  	language "C++"
	flags { "NoPCH", "StaticRuntime" }
	warnings "Extra"
	debugdir "."

  	filter { "platforms:*32" } architecture "x86"
  	filter { "platforms:*64" } architecture "x64"

	filter { "configurations:debug" }
		defines { "DEBUG" }
		symbols "On"
		objdir "build/debug"
		targetdir "bin/debug"

	filter { "configurations:debug", "action:vs*" }
		optimize "Debug"

	filter { "configurations:release" }
		defines { "NDEBUG" }
		optimize "Full"
		objdir "build/release"
		targetdir "bin/release"

	filter { "system:windows", "action:vs*"}
		defines { "_SECURE_SCL=0", "_CRT_SECURE_NO_WARNINGS=1" } 
		buildoptions { "/std:c++latest", "/permissive-" }
		flags { "MultiProcessorCompile" }

	filter {}

project "test"
    language "C++"
	kind "ConsoleApp"
    files { "test.cc", "nova-ecs/**.hh", "nova-ecs/**.cc" }
	includedirs { "nova-ecs" }
	location ("build/".._ACTION)
	targetname "test"
