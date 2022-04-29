#ifndef KT_TIMER_HPP
#define KT_TIMER_HPP


#ifdef _WIN32
#include "common/util/win32/timer.hpp"
typedef ktTimerWin32 timer;
#else
static_assert(false, "No timer implemented for current platform");
#endif


#endif
