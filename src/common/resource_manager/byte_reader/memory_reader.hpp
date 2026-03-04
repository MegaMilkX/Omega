#pragma once

#include "byte_reader.hpp"


class memory_reader : public byte_reader {
    void* m_data = nullptr;
    size_t m_size = 0;
    size_t m_cur = 0;
public:
    memory_reader(void* data, size_t size, extension hint = e_ext_unknown)
        : byte_reader(hint), m_data(data), m_size(size) {}

    size_t read(void* dst, size_t bytes) override;
    bool seek(int64_t offset, seek_origin origin) override;
    size_t tell() const override;
    size_t size() const override;
    bool is_valid() const override;
    bool is_eof() const override;
    byte_reader::memory_view try_slurp() override;
};