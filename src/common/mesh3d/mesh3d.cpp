#include "mesh3d.hpp"

#include "log/log.hpp"

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


#include "serialization/virtual_obuf.hpp"
#include "serialization/virtual_ibuf.hpp"
#include "serialization/serialization.hpp"
void Mesh3d::serialize(std::vector<unsigned char>& buf) {
    //LOG("Serializing mesh3d");
    vofbuf vof;
    vof.write<uint32_t>(vertex_count);
    vof.write<uint32_t>(index_count);
    vof.write<uint32_t>(arrays.size());
    for (auto& kv : arrays) {
        auto attrib_desc = VFMT::getAttribDesc(kv.first);
        //LOG(attrib_desc->name);

        vof.write_string(std::string(attrib_desc->name));
        vof.write_vector(kv.second, true);
    }
    if (index_count) {
        vof.write_vector(index_array, true);
    }
    buf.insert(buf.end(), vof.getData(), vof.getData() + vof.getSize());
    //LOG("Mesh3d serialization done.");
}
void Mesh3d::deserialize(const void* data, size_t sz) {
    //LOG("Deserializing Mesh3d");
    vifbuf vif((unsigned char*)data, sz);
    vertex_count = vif.read<uint32_t>();
    index_count = vif.read<uint32_t>();
    uint32_t array_count = vif.read<uint32_t>();
    for (int i = 0; i < array_count; ++i) {
        std::string attrib_name = vif.read_string();
        //LOG(attrib_name);
        std::vector<char> attrib_array;
        vif.read_vector(attrib_array);

        auto attrib_desc = VFMT::getAttribDesc(attrib_name.c_str());
        /*if (attrib_name == "Normal") {
            int attrib_size = attrib_desc->elem_size * attrib_desc->count;
            for (int i = 0; i < attrib_array.size(); i += attrib_size) {
                char* p = &attrib_array[i];
                if (attrib_desc->count == 3) {
                    gfxm::vec3 v = *(gfxm::vec3*)p;
                    LOG(v.x << ", " << v.y << ", " << v.z);
                }
                else if (attrib_desc->count == 2) {
                    gfxm::vec2 v = *(gfxm::vec2*)p;
                    LOG(v.x << ", " << v.y);
                }
            }
        }*/

        if (attrib_desc) {
            arrays[attrib_desc->global_id] = attrib_array;
        } else {
            LOG_ERR("Failed to get attribute description for " << attrib_name);
        }
    }
    if (index_count) {
        vif.read_vector(index_array);
    }
    //LOG("Mesh3d deserialization done.");
}
