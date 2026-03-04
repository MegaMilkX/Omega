#ifndef LOG_HPP
#define LOG_HPP

#include <string>
#include <iostream>
#include <sstream>
#include <thread>
#include <queue>
#include <mutex>
#include <fstream>
#include <ctime>

#include "log_consumer.hpp"
#include "math/gfxm.hpp"
//#include <util/filesystem/filesystem.hpp>

class Log {
private:
    std::unique_ptr<LogConsumer> stdout_consumer;
    std::unique_ptr<LogConsumer> file_consumer;
public:

    static Log* GetInstance();
    static void AddConsumer(LogConsumer* c);
    static void Write(const std::ostringstream& strm, LOG_TYPE type = LOG_INFO);
    static void Write(const std::string& str, LOG_TYPE type = LOG_INFO);
    static void Flush();
private:
    Log();
    ~Log();

    void _addConsumer(LogConsumer* c);
    void _write(const std::string& str, LOG_TYPE type);
    void _flush();

    bool working;
    std::mutex sync;
    std::mutex consumer_sync;
    std::queue<LogEntry> lines;
    std::thread thread_writer;
    std::vector<LogConsumer*> consumers;
};

#define MKSTR(LINE) \
(std::ostringstream() << LINE).str()

//#define LOG(LINE) std::cout << MKSTR(LINE) << std::endl;
#define LOG(LINE) Log::Write(std::ostringstream() << LINE);
#define LOG_WARN(LINE) Log::Write(std::ostringstream() << LINE, LOG_WARN);
#define LOG_ERR(LINE) Log::Write(std::ostringstream() << LINE, LOG_ERROR);
#define LOG_DBG(LINE) Log::Write(std::ostringstream() << LINE, LOG_DEBUG_INFO);

inline std::ostream& operator<< (std::ostream& stream, const gfxm::vec2& v) {
    stream << "[" << v.x << ", " << v.y << "]";
    return stream;
}
inline std::ostream& operator<< (std::ostream& stream, const gfxm::vec3& v) {
    stream << "[" << v.x << ", " << v.y << ", " << v.z << "]";
    return stream;
}
inline std::ostream& operator<< (std::ostream& stream, const gfxm::vec4& v) {
    stream << "[" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << "]";
    return stream;
}
inline std::ostream& operator<< (std::ostream& stream, const gfxm::quat& v) {
    stream << "[" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << "]";
    return stream;
}
inline std::ostream& operator<< (std::ostream& stream, const gfxm::mat3& v) {
    stream << v[0] << "\n" 
        << v[1] << "\n"
        << v[2];
    return stream;
}
inline std::ostream& operator<< (std::ostream& stream, const gfxm::mat4& v) {
    stream << v[0] << "\n" 
        << v[1] << "\n"
        << v[2] << "\n"
        << v[3];
    return stream;
}

#endif
