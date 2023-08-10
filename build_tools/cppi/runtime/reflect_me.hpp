#pragma once
/*
#include "reflect_me.auto.hpp"
#include <hahaha>

#include "game/game_test.hpp"

#define MYMACRO defined(TEST)

#if MYMACRO
#endif
*/
const char* c0 = "";
const char* c7 = "Hello";
const char* c5 = "\xDEAD";
const char* c1 = "\0";
const char* c2 = "\01";
const char* c3 = "\012";
const char* c4 = "\0123";
const char* c6 = "\UDEADBEEF";
const char* c8 = "\uBEEF";
const char* cm1 = "Mixed \n\t chars with \x41 Unicode \u0041 and \U00000041";
const char* cm2 = "Multiple Unicode: \u0041\u0042\u0043";
const char* cm3 = "Multiple Hexadecimal: \x41\x42\x43";
const char* ci1 = "\UFFFFFFFF";  // Unicode beyond limit
const char* ci2 = "\uD800";  // Unicode surrogate
//const char* ci3 = "\u00";  // Incomplete Unicode
//const char* ci4 = "\x";  // Incomplete hexadecimal
//const char* ci5 = "\";  // Incomplete simple escape sequence
const char* ci6 = "\g";  // Invalid escape sequence
const char* cs1 = "\n\r\t\v\b\f\a\?\"\'\"";  // \n, \r, \t, \v, \b, \f, \a, \?, \\, \', \"

const char* rc1 = R"()";
const char* rc2 = R"(Hello, World!)";
const char* rc3 = R"(Hello, \World\n)";
const char* rc4 = R"delimiter(Hello, \World\n)delimiter";
const char* rc5 = R"delimiter(Hello, \)delimiterWorld\n)delimiter";
const char* rc6 = R"delimiter(Hello, \World)delimiter\n)delimiter";
const char* rc7 = R"(Hello (World))";
const char* rc8 = R"(Hello "World")";

const char* con0 = "Hello"
				   ", World!";


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
class MyBase {};
[[cppi_class]];
class Foo {};

namespace MyNamespace {

[[cppi_enum]];
enum LimitedEnum {
	LIM_A, LIM_B, LIM_C
};

[[cppi_class]];
class MyClass : public MyBase, Foo {
public:

	[[cppi_enum]];
	enum STATE {
		ALIVE,
		DEAD
	};

	[[cppi_class]];
	struct Data {
		[[cppi_decl]]
		int integral;
		[[cppi_decl]]
		float floating;
		[[cppi_decl]]
		const char* pstr;
	};

	[[cppi_decl]]
	int value;
	[[cppi_decl, set("index")]]
	void setValue(int i) { value = i; }
	[[cppi_decl, get("index")]]
	int getValue() const { return value; }

	[[cppi_decl, set("color")]]
	void setColor(const gfxm::vec4& col);
	[[cppi_decl, get("color")]]
	const gfxm::vec4& getColor() const;

	[[cppi_decl, get("state")]]
	STATE getState() const;

	[[cppi_decl, set("invalid_set_only_prop")]]
	void setInvalidProp(float f);
	
};

}
