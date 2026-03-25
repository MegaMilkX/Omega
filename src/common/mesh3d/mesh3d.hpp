#pragma once

#include <map>
#include <vector>

#include "gpu/vertex_format.hpp"

class Mesh3d {
    int vertex_count = 0;
    int index_count = 0;
    std::map<VFMT::GUID, std::vector<char>> arrays;
    std::vector<char> index_array;
public:
    void clear();

    template<typename ATTRIB_T>
    void setAttribArray(VFMT::GUID guid_attrib, const std::vector<ATTRIB_T>& vec);
    void setAttribArray(VFMT::GUID guid_attrib, const void* data, size_t size);
    void setIndexArray(void* data, size_t size);

    size_t getAttribArraySize(VFMT::GUID guid_attrib) const;
    const void* getAttribArrayData(VFMT::GUID guid_attrib) const;

    bool hasIndices() const;
    size_t getAttribArrayCount() const;
    VFMT::GUID getAttribArrayGUID(int i) const;

    size_t getIndexArraySize() const;
    const void* getIndexArrayData() const;

    int getVertexCount() const;
    int getIndexCount() const;

    void serialize(std::vector<unsigned char>& buf);
    void deserialize(const void* data, size_t sz);
};

template<typename ATTRIB_T>
void Mesh3d::setAttribArray(VFMT::GUID guid_attrib, const std::vector<ATTRIB_T>& vec) {
    setAttribArray(guid_attrib, vec.data(), vec.size() * sizeof(vec[0]));
}

