#pragma once

#include <vector>

class vofbuf {
    std::vector<unsigned char> buf;
public:
    size_t getSize() {
        return buf.size();
    }
    unsigned char* getData() {
        return &buf[0];
    }

    template<typename T>
    void write(const T& value) {
        buf.insert(buf.end(), (unsigned char*)&value, ((unsigned char*)&value) + sizeof(value));
    }
    template<typename T>
    void write_vector(const std::vector<T>& vec, bool write_size) {
        unsigned char* begin = (unsigned char*)&vec[0];
        unsigned char* end = (unsigned char*)(&vec[0] + vec.size());
        if (write_size) {
            write<uint32_t>(vec.size());
        }
        buf.insert(buf.end(), begin, end);
    }
    void write_string_vector(const std::vector<std::string>& vec, bool write_size) {
        if (write_size) {
            write<uint32_t>(vec.size());
        }
        for (int i = 0; i < vec.size(); ++i) {
            write_string(vec[i]);
        }
    }
    void write_string(const std::string& str) {
        unsigned char* begin = (unsigned char*)&str[0];
        unsigned char* end = (unsigned char*)(&str[0] + str.size());
        write<uint32_t>(str.size());
        buf.insert(buf.end(), begin, end);
    }
};