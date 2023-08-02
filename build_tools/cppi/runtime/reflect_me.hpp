#pragma once

#include "reflect_me.auto.hpp"
#include <hahaha>

#include "game/game_test.hpp"

#define MYMACRO defined(TEST)

#if MYMACRO
#endif

[[cppi_enum]];
enum INPUT_DEVICE {
	INPUT_KEYBOARD,
	INPUT_XBOX,
	INPUT_PS	
};

[[cppi_class]];
class GuiInputFilePath {
public:
    GuiInputFilePath(
        const char* caption = "InputFilePath",
        std::string* output = 0,
        GUI_INPUT_FILE_TYPE type = GUI_INPUT_FILE_READ,
        const char* filter = "",
        const char* root_dir = ""
    ) : btn_browse("", guiLoadIcon ( "svg/entypo/folder.svg" ) ) {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);

        label.setOwner(this);
        label.setParent(this);
        box.setOwner(this);
        box.setParent(this);
        btn_browse.setOwner(this);
        btn_browse.setParent(this);
    }
};

[[cppi_class]];
class MyClass {
public:

	//[[cppi_class]];
	struct Data {
		[[cppi_decl]]
		int integral;
		[[cppi_decl]]
		float floating;
		[[cppi_decl]]
		const char* pstr;
	};

	//[[cppi_decl]]
	int value;
	//[[cppi_decl]]
	void function(int i) {}
	
};

