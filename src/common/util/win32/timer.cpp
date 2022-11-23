#include "timer.hpp"


ktTimerWin32::ktTimerWin32() {
    QueryPerformanceFrequency(&_freq);
}
void ktTimerWin32::start() {
    QueryPerformanceCounter(&_start);
}
float ktTimerWin32::stop() {
    QueryPerformanceCounter(&_end);
    _elapsed.QuadPart = _end.QuadPart - _start.QuadPart;
    _elapsed.QuadPart *= 1000000ll;
    _elapsed.QuadPart /= _freq.QuadPart;
    LONGLONG ms = _elapsed.QuadPart / 1000ll;
    return ms * .001f;
}