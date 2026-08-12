#ifndef GPU_MESH_DESC_HPP
#define GPU_MESH_DESC_HPP

#include <algorithm>
#include <vector>

#include <assert.h>
#include "log/log.hpp"
#include "gpu/gpu_types.hpp"
#include "gpu/types.hpp"
#include "gpu/vertex_format.hpp"
#include "gpu_buffer.hpp"
#include "gpu/intermediate_renderable_context.hpp"

#include "mesh3d/mesh3d.hpp"

#include "gpu/shader_set.hpp"


class gpuMeshDesc {
public:
    struct AttribDesc {
        VFMT::GUID guid;
        const gpuBuffer* buffer;
        int stride;
        int offset;
    };

    MESH_DRAW_MODE draw_mode = MESH_DRAW_TRIANGLES;
    GPU_MESH_DESC_TYPE mesh_type = GPU_MESH_DESC_TYPE::NONE;
private:
    std::vector<AttribDesc> attribs;
    const gpuBuffer* index_array = 0;
    int index_count     = 0;
    int vertex_count    = 0;

    int findAttribDescId(VFMT::GUID guid) const {
        if (attribs.empty()) {
            return -1;
        }

        int begin = 0;
        int end = attribs.size() - 1;
        while (begin <= end) {
            int middle = begin + (end - begin) / 2;
            if(guid == attribs[middle].guid) {
                return middle;
            } else if(guid < attribs[middle].guid) {
                end = middle - 1;
            } else if(guid > attribs[middle].guid) {
                begin = middle + 1;
            }
        }
        return -1;
    }
    const AttribDesc* findAttribDesc(VFMT::GUID guid) const {
        int id = findAttribDescId(guid);
        if (id == -1) {
            return 0;
        }
        return &attribs[id];
    }

public:
    virtual ~gpuMeshDesc() {}

    void clear() {
        vertex_count = 0;
        index_count = 0;
        index_array = 0;
        attribs.clear();
    }
    
    void setType(GPU_MESH_DESC_TYPE type) { mesh_type = type; }
    void setVertexCount(int vertex_count) { this->vertex_count = vertex_count; }
    void setIndexCount(int index_count) { this->index_count = index_count; }

    void setAttribArray(VFMT::GUID attrib_guid, const gpuBuffer* buffer, int stride = 0, int offset = 0) {
        { 
            auto dsc = VFMT::getAttribDesc(attrib_guid);
            if (dsc->primary) {
                size_t bufsz = buffer->getSize();
                vertex_count = bufsz / (dsc->elem_size * dsc->count);
            }
        }
        int found_idx = findAttribDescId(attrib_guid);
        if(found_idx == -1) {
            AttribDesc desc;
            desc.guid = attrib_guid;
            desc.buffer = buffer;
            desc.stride = stride;
            desc.offset = offset;
            attribs.push_back(desc);
            std::sort(attribs.begin(), attribs.end(), [](const AttribDesc& a, const AttribDesc& b) -> bool {
                return a.guid < b.guid;
            });
        } else {
            AttribDesc& desc = attribs[found_idx];
            desc.guid = attrib_guid;
            desc.buffer = buffer;
            desc.stride = stride;
            desc.offset = offset;
        }
    }
    void setIndexArray(const gpuBuffer* buffer) {
        index_array = buffer;
        index_count = index_array->getSize() / sizeof(uint32_t);
    }

    void setDrawMode(MESH_DRAW_MODE mode) {
        draw_mode = mode;
    }

    bool hasIndexArray() const {
        return index_array != 0;
    }

    int getLocalAttribId(VFMT::GUID attrib_guid) const {
        int id = findAttribDescId(attrib_guid);
        return id;
    }

    const AttribDesc* getAttribDesc(VFMT::GUID attrib_guid) const {
        return findAttribDesc(attrib_guid);
    }
    const AttribDesc& getLocalAttribDesc(int id) const {
        return attribs[id];
    }

    int getVertexCount() const {
        return vertex_count;
    }
    int getIndexCount() const {
        return index_count;
    }

    const gpuBuffer* getIndexBuffer() const {
        return index_array;
    }

    // add attributes from this mesh desc to target
    void merge(gpuMeshDesc* target, bool replace_existing_attribs) const {
        for (int i = 0; i < attribs.size(); ++i) {
            auto& a = attribs[i];
            if (!replace_existing_attribs && target->findAttribDescId(a.guid) >= 0) {
                continue;
            }
            target->setAttribArray(a.guid, a.buffer, a.stride);
        }
        if (index_array) {
            target->setIndexArray(index_array);
        }
    }

    void _bindVertexArray(VFMT::GUID attrib_guid, int location) const {
        int id = findAttribDescId(attrib_guid);
        if(id == -1) {
            assert(false);
            return;
        }
        const AttribDesc& desc = attribs[id];
        
        desc.buffer->bindArray();
        auto attrib_desc = VFMT::getAttribDesc(attrib_guid);
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(
            location,
            attrib_desc->count, attrib_desc->gl_type, attrib_desc->normalized,
            desc.stride, (void*)desc.offset /* offset */
        );
    }
    void _bindIndexArray() const {
        index_array->bindIndexArray();
    }

    void toMesh3d(Mesh3d* out) const {
        LOG("Converting gpu mesh to cpu mesh");
        for (int i = 0; i < attribs.size(); ++i) {
            auto& attr = attribs[i];
            LOG("toMesh3d: " << VFMT::getAttribDesc(attr.guid)->name);
            size_t buf_sz = attr.buffer->getSize();
            std::vector<unsigned char*> buf(buf_sz, 0);
            attr.buffer->getData(&buf[0]);
            out->setAttribArray(attr.guid, &buf[0], buf_sz);
        }
        if (index_array) {
            size_t buf_sz = index_array->getSize();
            std::vector<unsigned char*> buf(buf_sz, 0);
            index_array->getData(&buf[0]);
            out->setIndexArray(&buf[0], buf_sz);
        }
        LOG("Conversion done.");
    }

    void apply(GPU_INTERMEDIATE_PASS_DESC& pass) const;
};


#endif
