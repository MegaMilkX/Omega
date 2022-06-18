#include "mesh3d.hpp"


void Mesh3d::clear() {
    arrays.clear();
    index_array.clear();
    vertex_count = 0;
    index_count = 0;
}

void Mesh3d::setAttribArray(VFMT::GUID guid_attrib, void* data, size_t size) {
    auto& attrib_array = arrays[guid_attrib];
    attrib_array.resize(size);
    memcpy(attrib_array.data(), data, size);
    if (vertex_count == 0) {
        auto desc = VFMT::getAttribDesc(guid_attrib);
        int vert_size = desc->elem_size * desc->count;
        vertex_count = size / vert_size;
    }
}

void Mesh3d::setIndexArray(void* data, size_t size) {
    index_array.resize(size);
    memcpy(index_array.data(), data, size);
    index_count = size / 4;
    // TODO: maybe support 16 bit indices or even 64 bit
}

size_t Mesh3d::getAttribArraySize(VFMT::GUID guid_attrib) const {
    auto it = arrays.find(guid_attrib);
    if (it == arrays.end()) {
        return 0;
    }
    return it->second.size();
}
const void* Mesh3d::getAttribArrayData(VFMT::GUID guid_attrib) const {
    auto it = arrays.find(guid_attrib);
    if (it == arrays.end()) {
        return 0;
    }
    return it->second.data();
}

bool Mesh3d::hasIndices() const {
    return index_count != 0;
}
size_t Mesh3d::getAttribArrayCount() const {
    return arrays.size();
}
VFMT::GUID Mesh3d::getAttribArrayGUID(int i) const {
    auto it = arrays.begin();
    std::advance(it, i);
    if (it == arrays.end()) {
        return VFMT_INVALID_GUID;
    }
    return it->first;
}

size_t Mesh3d::getIndexArraySize() const {
    return index_array.size();
}
const void* Mesh3d::getIndexArrayData() const {
    return index_array.data();
}

int Mesh3d::getVertexCount() const {
    return vertex_count;
}
int Mesh3d::getIndexCount() const {
    return index_count;
}

class vofbuf {
    std::vector<unsigned char> buf;
public:
    size_t getSize() {
        return buf.size();
    }
    unsigned char* getData() {
        return &buf[0];
    }

    template<typename T>
    void write(const T& value) {
        buf.insert(buf.end(), (unsigned char*)&value, ((unsigned char*)&value) + sizeof(value));
    }
    template<typename T>
    void write_vector(const std::vector<T>& vec, bool write_size) {
        unsigned char* begin = (unsigned char*)&vec[0];
        unsigned char* end = (unsigned char*)(&vec[0] + vec.size());
        if (write_size) {
            write<uint32_t>(vec.size());
        }
        buf.insert(buf.end(), begin, end);
    }
    void write_string(const std::string& str) {
        unsigned char* begin = (unsigned char*)&str[0];
        unsigned char* end = (unsigned char*)(&str[0] + str.size());
        write<uint32_t>(str.size());
        buf.insert(buf.end(), begin, end);
    }
};

class vifbuf {
    unsigned char* buf;
    size_t size;
    unsigned char* begin;
    unsigned char* end;
    unsigned char* cur;
public:
    vifbuf(unsigned char* buf, size_t size)
    : buf(buf), size(size), begin(buf), end(buf + size), cur(buf) {

    }
    bool isEof() const {
        return cur >= end;
    }
    template<typename T>
    T read() {
        T ret = { 0 };
        ret = *(T*)cur;
        cur += sizeof(T);
        return ret;
    }
    template<typename T>
    void read_vector(std::vector<T>& vec) {
        uint32_t count = read<uint32_t>();
        vec.resize(count);
        memcpy(&vec[0], cur, count * sizeof(T));
        cur += count * sizeof(T);
    }
    std::string read_string() {
        uint32_t len = read<uint32_t>();
        std::string ret(len, 0);
        memcpy(&ret[0], cur, len);
        cur += len;
        return ret;
    }
};

#include "serialization/serialization.hpp"
void Mesh3d::serialize(std::vector<unsigned char>& buf) {
    vofbuf vof;
    vof.write<uint32_t>(vertex_count);
    vof.write<uint32_t>(index_count);
    vof.write<uint32_t>(arrays.size());
    for (auto& kv : arrays) {
        auto attrib_desc = VFMT::getAttribDesc(kv.first);
        vof.write_string(std::string(attrib_desc->name));
        vof.write_vector(kv.second, true);
    }
    if (index_count) {
        vof.write_vector(index_array, true);
    }
    buf.insert(buf.end(), vof.getData(), vof.getData() + vof.getSize());
}
void Mesh3d::deserialize(const void* data, size_t sz) {
    vifbuf vif((unsigned char*)data, sz);
    vertex_count = vif.read<uint32_t>();
    index_count = vif.read<uint32_t>();
    uint32_t array_count = vif.read<uint32_t>();
    for (int i = 0; i < array_count; ++i) {
        std::string attrib_name = vif.read_string();
        std::vector<char> attrib_array;
        vif.read_vector(attrib_array);

        auto attrib_desc = VFMT::getAttribDesc(attrib_name.c_str());
        if (attrib_desc) {
            arrays[attrib_desc->global_id] = attrib_array;
        }
    }
    if (index_count) {
        vif.read_vector(index_array);
    }
}
