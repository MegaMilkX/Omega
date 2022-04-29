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