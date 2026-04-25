project "gui_test"
	kind		"ConsoleApp"
	language	"C++"
	
	debugdir	(WORKING_DIR)
	
	vpaths { ["*"] = "./gui_test/**" }
	
	files {
        "./gui_test/**.cpp",
        "./gui_test/**.c",
        "./gui_test/**.cxx",
        "./gui_test/**.h",
        "./gui_test/**.hpp",
        "../build/resource.rc",
    }
	
	includedirs {
        "./gui_test/",
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