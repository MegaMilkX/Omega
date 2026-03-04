#pragma once

#include "log_consumer.hpp"


class StdoutLogConsumer : public LogConsumer {
public:
    ~StdoutLogConsumer();
    void consume(const LogEntry& e) override;
    void flush() override;
};

