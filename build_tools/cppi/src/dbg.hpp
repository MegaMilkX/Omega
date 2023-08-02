#pragma once

#define DBG_BLUE      0x0001 // text color contains blue.
#define DBG_GREEN     0x0002 // text color contains green.
#define DBG_RED       0x0004 // text color contains red.
#define DBG_INTENSITY 0x0008 // text color is intensified.
#define DBG_BG_BLUE      0x0010 // background color contains blue.
#define DBG_BG_GREEN     0x0020 // background color contains green.
#define DBG_BG_RED       0x0040 // background color contains red.
#define DBG_BG_INTENSITY 0x0080 // background color is intensified.


void dbg_printf_color(const char* fmt, unsigned short color, ...);