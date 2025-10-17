#pragma once

#define DBG_BLUE      0x0001 // text color contains blue.
#define DBG_GREEN     0x0002 // text color contains green.
#define DBG_RED       0x0004 // text color contains red.
#define DBG_INTENSITY 0x0008 // text color is intensified.
#define DBG_BG_BLUE      0x0010 // background color contains blue.
#define DBG_BG_GREEN     0x0020 // background color contains green.
#define DBG_BG_RED       0x0040 // background color contains red.
#define DBG_BG_INTENSITY 0x0080 // background color is intensified.

#define DBG_YELLOW DBG_RED | DBG_GREEN

#define DBG_WHITE   DBG_RED | DBG_GREEN | DBG_BLUE
#define DBG_KEYWORD DBG_BLUE | DBG_INTENSITY
#define DBG_USER_DEFINED_TYPE DBG_RED | DBG_GREEN | DBG_INTENSITY
#define DBG_LITERAL DBG_GREEN | DBG_BLUE | DBG_INTENSITY
#define DBG_STRING_LITERAL DBG_RED | DBG_BLUE | DBG_INTENSITY
#define DBG_IDENTIFIER DBG_RED | DBG_GREEN | DBG_INTENSITY
#define DBG_OPERATOR DBG_WHITE
#define DBG_COMMENT DBG_GREEN | DBG_INTENSITY

#define DBG_ERROR DBG_WHITE | DBG_BG_RED
#define DBG_NOT_IMPLEMENTED DBG_WHITE | DBG_BG_RED | DBG_BG_BLUE


void dbg_printf_color(const char* fmt, unsigned short color, ...);
void dbg_printf_color_indent(const char* fmt, int indent, unsigned short color, ...);