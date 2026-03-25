#include "vector_writer.hpp"
#include <assert.h>


vector_writer::vector_writer(std::vector<unsigned char>& vec)
: vec(vec), cur(vec.size()) {}

size_t vector_writer::write(const void* buf, size_t sz) {
    size_t required_size = cur + sz;
    if (vec.size() < required_size) {
        vec.resize(required_size);
    }
    memcpy(&vec[cur], buf, sz);
    cur += sz;
    return sz;
}

bool vector_writer::seek(int64_t offset, seek_origin origin) {
    switch (origin) {
    case seek_cur: {
        int64_t new_cur = cur + offset;
        if (new_cur > vec.size() || new_cur < 0) {
            return false;
        }
        cur = new_cur;
        return true;
    }
    case seek_end: {
        int64_t new_cur = vec.size() + offset;
        if (new_cur > vec.size() || new_cur < 0) {
            return false;
        }
        cur = new_cur;
        return true;
    }
    case seek_set:
        if (offset > vec.size() || offset < 0) {
            return false;
        }
        cur = offset;
        return true;
    }
    assert(false);
    return false;
}

size_t vector_writer::tell() const {
    return cur;
}