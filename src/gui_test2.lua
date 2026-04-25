project "gui_test2"
	kind		"ConsoleApp"
	language	"C++"
	
	debugdir	(WORKING_DIR)
	
	vpaths { ["*"] = "./gui_test2/**" }
	
	files {
        "./gui_test2/**.cpp",
        "./gui_test2/**.c",
        "./gui_test2/**.cxx",
        "./gui_test2/**.h",
        "./gui_test2/**.hpp",
        "../build/resource.rc",
    }
	
	includedirs {
        "./gui_test2/",
        "./common/",
        "./lib/",
        "./lib/assimp/include/",
        "./lib/freetype-2.10.0/include/",
        "./lib/FastNoiseSIMD/",
    }
	
	defines {}	
	
	links {
        "shlwapi",
        "OpenGL32",
        "hid",
        "setupapi",
        "xaudio2",
        "common",
        "assimp-vc143-mt",
		"zlibstatic",
        "freetype",
        "FastNoiseSIMD",
    }
	
	filter "configurations:Debug"
        targetdir (BUILD_DIR .. "/debug")
		libdirs {
			"./../build/lib/RelWithDebInfo"
		}

    filter "configurations:Release"
        targetdir (BUILD_DIR .. "/release")
		libdirs {
			"./../build/lib/RelWithDebInfo"
		}
		
	filter {}