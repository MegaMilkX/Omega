#pragma once

#include <string>
#include "byte_writer.hpp"


class file_writer : public byte_writer {
    FILE* file = nullptr;

    int seek_origin_to_c_constant(seek_origin origin);
public:
    file_writer(FILE* file);
    ~file_writer();

    size_t write(const void* buf, size_t sz) override;
    bool seek(int64_t offset, seek_origin origin) override;
    size_t tell() const override;
};