#pragma once

#include <fstream>
#include "log_consumer.hpp"


class FileLogConsumer : public LogConsumer {
    std::ofstream file;
public:
    FileLogConsumer(const std::string& fname = "");
    ~FileLogConsumer();
    void consume(const LogEntry& e) override;
    void flush() override;
};

