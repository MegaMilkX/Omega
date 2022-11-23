#ifndef KT_TIMER_WIN32_HPP
#define KT_TIMER_WIN32_HPP

#define NO_MIN_MAX
#include <windows.h>
#include <stdint.h>


class ktTimerWin32 {
private:
    LARGE_INTEGER _freq;
    LARGE_INTEGER _start, _end;
    LARGE_INTEGER _elapsed;
public:
    ktTimerWin32();
    void start();
    float stop();
};


#endif
