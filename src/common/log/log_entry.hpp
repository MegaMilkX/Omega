#pragma once

#include <time.h>
#include <string>


enum LOG_TYPE {
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_DEBUG_INFO,
    LOG_DEBUG_WARN,
    LOG_DEBUG_ERROR
};
inline std::string logTypeToString(LOG_TYPE type) {
    std::string str;
    switch(type) {
    case LOG_INFO:
        str = "INFO";
        break;
    case LOG_WARN:
        str = "WARN";
        break;
    case LOG_ERROR:
        str = "ERR ";
        break;
    case LOG_DEBUG_INFO:
        str = "DINF";
        break;
    case LOG_DEBUG_WARN:
        str = "DWRN";
        break;
    case LOG_DEBUG_ERROR:
        str = "DERR";
        break;
    }
    return str;
}


struct LogEntry {
    LOG_TYPE type;
    time_t t;
    unsigned long thread_id;
    std::string line;
};

