#pragma once

#include <vector>
#include "byte_writer.hpp"


class vector_writer : public byte_writer {
    std::vector<unsigned char>& vec;
    size_t cur = 0;
public:
    vector_writer(std::vector<unsigned char>& vec);

    size_t write(const void* buf, size_t sz) override;
    bool seek(int64_t offset, seek_origin origin) override;
    size_t tell() const override;
};
