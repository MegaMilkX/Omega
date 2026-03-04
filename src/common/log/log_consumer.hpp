#pragma once

#include "log_entry.hpp"


class LogConsumer {
public:
    virtual ~LogConsumer() {}
    virtual void consume(const LogEntry& e) = 0;
    virtual void flush() {};
};

