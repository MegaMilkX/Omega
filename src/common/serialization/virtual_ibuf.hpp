#pragma once

#include <stdint.h>

class vifbuf {
    unsigned char* buf;
    size_t size;
    unsigned char* begin;
    unsigned char* end;
    unsigned char* cur;
public:
    vifbuf(unsigned char* buf, size_t size)
        : buf(buf), size(size), begin(buf), end(buf + size), cur(buf) {

    }
    bool isEof() const {
        return cur >= end;
    }
    template<typename T>
    T read() {
        T ret = { 0 };
        ret = *(T*)cur;
        cur += sizeof(T);
        return ret;
    }
    template<typename T>
    void read_vector(std::vector<T>& vec) {
        uint32_t count = read<uint32_t>();
        vec.resize(count);
        memcpy(&vec[0], cur, count * sizeof(T));
        cur += count * sizeof(T);
    }
    void read_string_vector(std::vector<std::string>& vec) {
        uint32_t count = read<uint32_t>();
        vec.resize(count);
        for (int i = 0; i < count; ++i) {
            vec[i] = read_string();
        }
    }
    std::string read_string() {
        uint32_t len = read<uint32_t>();
        std::string ret(len, 0);
        memcpy(&ret[0], cur, len);
        cur += len;
        return ret;
    }
};