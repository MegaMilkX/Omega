WORKSPACE 		= "OmegaEngine"
WORKING_DIR 	= "../runtime_data/"
PREMAKE_DIR		= "../build/premake/"
BUILD_DIR 		= "../bin/"

workspace(WORKSPACE)
	location(PREMAKE_DIR)
	
	configurations 			{ "Debug", "Release" }
	platforms				{ "x64" }
	cppdialect				"C++20"
	staticruntime			"on"		-- MT/MTd
	multiprocessorcompile 	"on"		-- /MP
	characterset			"MBCS"		-- Multi-Byte Character Set (WINAPI ***A functions)
	
	defines {
		"_CRT_SECURE_NO_WARNINGS",		-- No "unsafe strlen" warnings or errors
		"NOMINMAX",						-- 
	}
	
	filter "configurations:Debug"
		runtime 	"Debug"
		optimize 	"off"
		symbols 	"on"
		
	filter "configurations:Release"
		runtime		"Release"
		optimize 	"on"
		symbols 	"on"
		defines {
			"NDEBUG",		-- No asserts
		}
		
	filter "action:vs*"
		buildoptions { "/bigobj" }
		
	filter {}
	
	group ""
	include("common.lua")
	include("omega.lua")
	include("gui_test.lua")
	include("gui_test2.lua")

workspace(WORKSPACE)
	startproject	"omega"