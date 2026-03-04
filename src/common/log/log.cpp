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
    /*
    thread_writer = std::thread([this](){
        tm ptm = {0};
        time_t t = time(0);
        localtime_s(&ptm, &t);
        char buffer[64];
        strftime(buffer, 64, "%d%m%Y", &ptm);
        std::string fname = win32GetThisModuleName() + "_" + std::string(buffer);

        fsCreateDirRecursive(fsGetModuleDir() + "\\log");

        //std::ios::openmode mode = std::ios::out | std::ios::app;
        std::ios::openmode mode = std::ios::out | std::ios::trunc;
        std::ofstream f(fsGetModuleDir() + "\\log\\" + fname + ".log", mode);

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
            while(!lines_copy.empty()) {
                LogEntry e = lines_copy.front();
                tm ptm = {0};
                localtime_s(&ptm, &e.t);
                char buffer[32];
                strftime(buffer, 32, "%H:%M:%S", &ptm); 
                lines_copy.pop();

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
                    << " | " << buffer 
                    << " | " << std::hex << std::uppercase << e.thread_id 
                    << ": " << e.line 
                    << std::endl).str();
                f << str;
                std::cout << str;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } while(1);

        f << "\n\n\n";

        f.flush();
        f.close();
    });*/
    
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

