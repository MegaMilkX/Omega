#include "file_writer.hpp"
#include <assert.h>


int file_writer::seek_origin_to_c_constant(seek_origin origin) {
    switch(origin) {
    case seek_set: return SEEK_SET;
    case seek_cur: return SEEK_CUR;
    case seek_end: return SEEK_END;
    };
    assert(false);
    return -1;
}

file_writer::file_writer(FILE* file)
: file(file) {}

file_writer::~file_writer() {}

size_t file_writer::write(const void* buf, size_t sz) {
    return fwrite(buf, 1, sz, file);
}

bool file_writer::seek(int64_t offset, seek_origin origin) {
    return 0 == fseek(file, offset, seek_origin_to_c_constant(origin));
}

size_t file_writer::tell() const {
    return ftell(file);
}
