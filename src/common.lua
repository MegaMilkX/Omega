project "common"
	kind		"StaticLib"
	language	"C++"
	
	vpaths { ["*"] = "./common/**" }
	
	files {
		"./common/**.cpp",
		"./common/**.c",
		"./common/**.cxx",
		"./common/**.h",
		"./common/**.hpp"		
	}
	
	includedirs {
		"./common/",
        "./lib/",
        "./lib/FastNoiseSIMD/",
        "./lib/freetype-2.10.0/include/",
        "./lib/assimp/include/",
	}
	
	defines {}
	
	links {
        "shlwapi",
        "OpenGL32",
        "hid",
        "setupapi",
        "xaudio2",
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