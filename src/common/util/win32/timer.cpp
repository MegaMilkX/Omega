#include "timer.hpp"


ktTimerWin32::ktTimerWin32() {
    QueryPerformanceFrequency(&_freq);
    started = false;
}
void ktTimerWin32::start() {
    QueryPerformanceCounter(&_start);
    started = true;
}
float ktTimerWin32::stop() {
    started = false;
    QueryPerformanceCounter(&_end);
    uint64_t elapsedMicrosec = ((_end.QuadPart - _start.QuadPart) * 1000000LL) / _freq.QuadPart;
    return (float)elapsedMicrosec * .000001f;
}
bool ktTimerWin32::is_started() {
    return started;
}