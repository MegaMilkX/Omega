#pragma once

#include <stdio.h>
#include <string>
#include "byte_reader.hpp"


class file_reader : public byte_reader {
    FILE* file = nullptr;
    mutable size_t m_size = 0;
    unsigned char* slurped = nullptr;
    
    size_t get_size() const;
    int seek_origin_to_c_constant(seek_origin origin);
public:
    file_reader();
    file_reader(const std::string& fname, extension hint = e_ext_unknown);
    ~file_reader();

    bool open(const std::string& fname, extension hint = e_ext_unknown);

    size_t read(void* dst, size_t bytes) override;
    bool seek(int64_t offset, seek_origin origin) override;
    size_t tell() const override;
    size_t size() const override;
    bool is_valid() const override;
    bool is_eof() const override;
    byte_reader::memory_view try_slurp() override;
};

