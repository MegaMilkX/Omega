#pragma once

#include <vector>
#include <stdio.h>

template<typename T>
size_t fwrite_length(const std::vector<T>& v, FILE* f) {
    uint32_t count = v.size();
    return fwrite(&count, sizeof(count), 1, f);
}
template<typename T>
size_t fwrite_vector(const std::vector<T>& v, FILE* f, bool write_length = true) {
    size_t ret = 0;
    uint32_t count = v.size();
    if (write_length) {
        ret = fwrite_length(v, f);
    }
    return ret + fwrite(v.data(), sizeof(v[0]), count, f);
}
inline size_t fwrite_string(const std::string& str, FILE* f) {
    return fwrite(str.data(), sizeof(str[0]), str.size(), f);
}