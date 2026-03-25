#pragma once

#include <string>


class byte_writer {
public:
    enum seek_origin {
        seek_set,
        seek_cur,
        seek_end
    };

    virtual ~byte_writer() {}

    virtual size_t write(const void* buf, size_t sz) = 0;

    virtual bool seek(int64_t offset, seek_origin origin) = 0;
    virtual size_t tell() const = 0;

    template<typename T>
    size_t write(const T& v) {
        return write(&v, sizeof(T));
    }
    size_t write_string(const std::string& str) {
        uint32_t len = str.size();
        size_t ret = 0;
        ret += write(len);
        ret += write(str.data(), len);
        return ret;
    }
    /*
    // TODO: T needs to be IWriteable or something I guess
    // Also when embedded need to write to vector<byte> first to store the size so it could be skipped
    // but this is the base for vector_writer, annoying
    template<typename T>
    size_t write_resource_ref(const ResourceRef<T>& ref) {
        size_t ret = 0;

        uint8_t embedded = ref.getResourceId().empty() ? 1 : 0;
        ret += write<uint8_t>(embedded);

        if (embedded) {
            ref->save(*this);
        } else {
            std::string res_id = ref.getResourceId();
            ret += write_string(res_id);
        }
    }*/
};

