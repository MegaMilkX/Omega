#pragma once

#include <stdint.h>
#include <string>
#include "extensions.hpp"


class byte_reader {
protected:
    extension extension_hint;
    std::string filename_hint_;
public:
    struct memory_view {
        const uint8_t* data;
        size_t size;

        operator bool() const {
            return bool(data);
        }
    };
    enum seek_origin {
        seek_set,
        seek_cur,
        seek_end
    };

    byte_reader(extension hint = e_ext_unknown, const std::string& filename_hint = "")
        : extension_hint(hint), filename_hint_(filename_hint) {}
    virtual ~byte_reader() = default;

    virtual size_t read(void* dst, size_t bytes) = 0;
    virtual bool seek(int64_t offset, seek_origin origin) = 0;
    virtual size_t tell() const = 0;
    virtual size_t size() const = 0;
    virtual bool is_valid() const = 0;
    virtual bool is_eof() const = 0;
    virtual memory_view try_slurp() = 0;

    extension hint() const { return extension_hint; }
    const std::string& filename_hint() const { return filename_hint_; }

    operator bool() const {
        return is_valid();
    }
};

