#include "dbg.hpp"

#include <stdio.h>
#include <Windows.h>

static HANDLE get_console_handle() {
    return GetStdHandle(STD_OUTPUT_HANDLE);
}
void dbg_printf_color(const char* fmt, unsigned short color, ...) {
    static HANDLE h = get_console_handle();

    SetConsoleTextAttribute(h, color);
    va_list args;
    va_start(args, color);
    vprintf(fmt, args);
    va_end(args);
    SetConsoleTextAttribute(h, 8);
}
