#include "file_log_consumer.hpp"

#include "platform/win32/module.hpp"
#include "filesystem/filesystem.hpp"


FileLogConsumer::FileLogConsumer(const std::string& fname) {
    std::string filename = fname;
    if(filename.empty()) {
        tm ptm = {0};
        time_t t = time(0);
        localtime_s(&ptm, &t);
        char buffer[64];
        strftime(buffer, 64, "%d%m%Y", &ptm);
        std::string name = win32GetThisModuleName() + "_" + std::string(buffer);

        fsCreateDirRecursive(fsGetModuleDir() + "\\log");
        filename = fsGetModuleDir() + "\\log\\" + name + ".log";
    }
    //std::ios::openmode mode = std::ios::out | std::ios::app;
    std::ios::openmode mode = std::ios::out | std::ios::trunc;
    file = std::ofstream(filename, mode);
}
FileLogConsumer::~FileLogConsumer() {
    file << "\n\n\n";
    file.flush();
    file.close();
}

void FileLogConsumer::consume(const LogEntry& e) {
    tm ptm = {0};
    localtime_s(&ptm, &e.t);
    char buffer[32];
    strftime(buffer, 32, "%H:%M:%S", &ptm); 

    std::string str = static_cast<std::ostringstream&>(
        std::ostringstream() << logTypeToString(e.type) 
        << "|" << buffer 
        << "|" << std::hex << std::uppercase << e.thread_id 
        << ": " << e.line 
        << std::endl).str();
    file << str;
}
void FileLogConsumer::flush() {
    file.flush();
}

