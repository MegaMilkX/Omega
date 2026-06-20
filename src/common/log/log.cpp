#include "log.hpp"

#define WIN32_LEAN_AND_MEAN
#include "platform/win32/module.hpp"
#include "filesystem/filesystem.hpp"

#include "stdout_log_consumer.hpp"
#include "file_log_consumer.hpp"


Log* Log::GetInstance() {
    static Log fl;
    return &fl;
}
void Log::Write(const std::ostringstream& strm, LOG_TYPE type) {
    GetInstance()->_write(strm.str(), type);
}
void Log::Write(const std::string& str, LOG_TYPE type) {
    GetInstance()->_write(str, type);
}
void Log::Flush() {
    GetInstance()->_flush();
}
void Log::AddConsumer(LogConsumer* c) {
    GetInstance()->_addConsumer(c);
}


Log::Log()
: working(true) {    
    stdout_consumer.reset(new StdoutLogConsumer());
    _addConsumer(stdout_consumer.get());
    file_consumer.reset(new FileLogConsumer());
    _addConsumer(file_consumer.get());

    thread_writer = std::thread([this](){
        do {
            std::queue<LogEntry> lines_copy;
            {
                std::lock_guard<std::mutex> lock(sync);
                if(!working && lines.empty())
                    break;
                
                lines_copy = lines;
                while(!lines.empty()) {
                    lines.pop();
                }
            }

            {
                std::lock_guard<std::mutex> lock(consumer_sync);
                bool anything_written = false;
                while(!lines_copy.empty()) {
                    for (int i = 0; i < consumers.size(); ++i) {
                        consumers[i]->consume(lines_copy.front());
                    }
                    lines_copy.pop();
                    anything_written = true;
                }
                if(anything_written) {
                    for (int i = 0; i < consumers.size(); ++i) {
                        consumers[i]->flush();
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } while(1);

        for (int i = 0; i < consumers.size(); ++i) {
            consumers[i]->flush();
        }
    });
}
Log::~Log() {
    working = false;
    if(thread_writer.joinable()) {
        thread_writer.join();
    }
}

void Log::_addConsumer(LogConsumer* c) {
    std::lock_guard<std::mutex> lock(consumer_sync);
    consumers.push_back(c);
}
void Log::_write(const std::string& str, LOG_TYPE type) {
    std::lock_guard<std::mutex> lock(sync);
    lines.push(LogEntry{
        type,
        time(0),
        GetCurrentThreadId(),
        str
    });
}
void Log::_flush() {
    // TODO: lol
}

