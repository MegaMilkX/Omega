#include <algorithm>
#include "memory_reader.hpp"


size_t memory_reader::read(void* dst, size_t bytes) {
    size_t count = std::min(bytes, m_size - m_cur);
    memcpy(dst, (uint8_t*)m_data + m_cur, count);
    m_cur += count;
    return count;
}

bool memory_reader::seek(int64_t offset, seek_origin origin) {
    switch (origin) {
    case seek_set:
        if(offset < 0) return false;
        if(offset > m_size) return false;
        m_cur = offset;
        return true;
    case seek_cur: {
        size_t new_cur = m_cur + offset;
        if(new_cur < 0) return false;
        if(new_cur > m_size) return false;
        m_cur = new_cur;
        return true;
    }
    case seek_end: {
        size_t new_cur = m_size + offset;
        if(new_cur < 0) return false;
        if(new_cur > m_size) return false;
        m_cur = new_cur;
        return true;
    }
    }
}

size_t memory_reader::tell() const {
    return m_cur;
}

size_t memory_reader::size() const {
    return m_size;
}

bool memory_reader::is_valid() const {
    return m_data && m_size;
}

bool memory_reader::is_eof() const {
    return m_cur >= m_size;
}

byte_reader::memory_view memory_reader::try_slurp() {
    return memory_view {
        .data = (const uint8_t*)m_data,
        .size = m_size
    };
}

