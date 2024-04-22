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

#include "math/gfxm.hpp"
//#include <util/filesystem/filesystem.hpp>

class Log {
public:
    enum Type {
        LOG_INFO,
        LOG_WARN,
        LOG_ERROR,
        LOG_DEBUG_INFO,
        LOG_DEBUG_WARN,
        LOG_DEBUG_ERROR
    };

    static Log* GetInstance();
    static void Write(const std::ostringstream& strm, Type type = LOG_INFO);
    static void Write(const std::string& str, Type type = LOG_INFO);
private:
    Log();
    ~Log();

    void _write(const std::string& str, Type type);

    std::string _typeToString(Type type);

    struct entry {
        Type type;
        time_t t;
        unsigned long thread_id;
        std::string line;
    };

    bool working;
    std::mutex sync;
    std::queue<entry> lines;
    std::thread thread_writer;
};

#define MKSTR(LINE) \
(std::ostringstream() << LINE).str()

//#define LOG(LINE) std::cout << MKSTR(LINE) << std::endl;
#define LOG(LINE) Log::Write(std::ostringstream() << LINE);
#define LOG_WARN(LINE) Log::Write(std::ostringstream() << LINE, Log::LOG_WARN);
#define LOG_ERR(LINE) Log::Write(std::ostringstream() << LINE, Log::LOG_ERROR);
#define LOG_DBG(LINE) Log::Write(std::ostringstream() << LINE, Log::LOG_DEBUG_INFO);

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
