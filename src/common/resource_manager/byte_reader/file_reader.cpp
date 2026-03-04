#include <assert.h>
#include "file_reader.hpp"


size_t file_reader::get_size() const {
    size_t t = ftell(file);
    fseek(file, 0, SEEK_END);
    size_t s = ftell(file);
    fseek(file, t, SEEK_SET);
    return s;
}
int file_reader::seek_origin_to_c_constant(seek_origin origin) {
    switch(origin) {
    case seek_set: return SEEK_SET;
    case seek_cur: return SEEK_CUR;
    case seek_end: return SEEK_END;
    };
    assert(false);
    return -1;
}

file_reader::file_reader()
    : file(0), m_size(0), slurped(nullptr) {}
file_reader::file_reader(const std::string& fname, extension hint)
: byte_reader(hint, fname), file(0), m_size(0), slurped(nullptr) {
    file = fopen(fname.c_str(), "rb");
}
file_reader::~file_reader() {
    if (file) fclose(file);
    if (slurped) delete[] slurped;
}

bool file_reader::open(const std::string& fname, extension hint) {
    if (file) fclose(file);
    if (slurped) delete[] slurped;
    file = fopen(fname.c_str(), "rb");
    byte_reader::extension_hint = hint;
    byte_reader::filename_hint_ = fname;
    return bool(file);
}

size_t file_reader::read(void* dst, size_t bytes) {
    return fread(dst, 1, bytes, file);
}
bool file_reader::seek(int64_t offset, seek_origin origin) {
    return 0 == fseek(file, offset, seek_origin_to_c_constant(origin));
}
size_t file_reader::tell() const {
    return ftell(file);
}
size_t file_reader::size() const {
    if (m_size == 0) {
        m_size = get_size();
    }
    return m_size;
}
bool file_reader::is_valid() const {
    return file != nullptr;
}
bool file_reader::is_eof() const {
    return ftell(file) >= size();
}
byte_reader::memory_view file_reader::try_slurp() {
    if (slurped) {
        return memory_view{
            .data = slurped,
            .size = size()
        };
    }

    size_t s = size();
    slurped = new unsigned char[s];
    assert(slurped);
    seek(0, seek_set);
    read(slurped, s);

    return memory_view{
        .data = slurped,
        .size = s
    };
}

