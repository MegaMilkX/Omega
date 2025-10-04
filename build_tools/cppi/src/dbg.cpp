#include "dbg.hpp"

#include <stdio.h>
#include <Windows.h>

static HANDLE get_console_handle() {
    return GetStdHandle(STD_OUTPUT_HANDLE);
}
void dbg_printf_color(const char* fmt, unsigned short color, ...) {
    static HANDLE h = get_console_handle();
    
    // Save current console info
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(h, &consoleInfo);
    WORD saved_attributes = consoleInfo.wAttributes;

    SetConsoleTextAttribute(h, color);
    va_list args;
    va_start(args, color);
    vprintf(fmt, args);
    va_end(args);
    //SetConsoleTextAttribute(h, 8);
    SetConsoleTextAttribute(h, saved_attributes);
}

void dbg_printf_color_indent(const char* fmt, int indent, unsigned short color, ...) {
    for(int i = 0; i < indent; ++i) printf("  ");

    static HANDLE h = get_console_handle();

    // Save current console info
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(h, &consoleInfo);
    WORD saved_attributes = consoleInfo.wAttributes;

    SetConsoleTextAttribute(h, color);
    va_list args;
    va_start(args, color);
    vprintf(fmt, args);
    va_end(args);
    //SetConsoleTextAttribute(h, 8);
    SetConsoleTextAttribute(h, saved_attributes);
}

