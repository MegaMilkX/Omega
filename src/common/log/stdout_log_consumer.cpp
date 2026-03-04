#include "stdout_log_consumer.hpp"

#include <iostream>
#include <sstream>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>


StdoutLogConsumer::~StdoutLogConsumer() {
    std::cout << std::flush;
}
void StdoutLogConsumer::consume(const LogEntry& e) {
    tm ptm = {0};
    localtime_s(&ptm, &e.t);
    char buffer[32];
    strftime(buffer, 32, "%H:%M:%S", &ptm);

    HANDLE  hConsole;	
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if(e.type == LOG_TYPE::LOG_INFO) {
        SetConsoleTextAttribute(hConsole, 7);
    } else if(e.type == LOG_TYPE::LOG_WARN) {
        SetConsoleTextAttribute(hConsole, 0xE);
    } else if(e.type == LOG_TYPE::LOG_ERROR) {
        SetConsoleTextAttribute(hConsole, 0xC);
    } else if(e.type == LOG_TYPE::LOG_DEBUG_INFO) {
        SetConsoleTextAttribute(hConsole, 0x0001 | 0x0002 | 0x0008);
    }
    std::string str = static_cast<std::ostringstream&>(
        std::ostringstream() << logTypeToString(e.type) 
        << "|" << buffer 
        << "|" << std::hex << std::uppercase << e.thread_id 
        << ": " << e.line 
        << std::endl).str();
    std::cout << str;
}
void StdoutLogConsumer::flush() {
    std::cout << std::flush;
}

