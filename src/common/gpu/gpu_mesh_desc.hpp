#ifndef GPU_MESH_DESC_HPP
#define GPU_MESH_DESC_HPP

#include <algorithm>
#include <vector>

#include <assert.h>
#include "log/log.hpp"
#include "gpu/vertex_format.hpp"
#include "gpu_buffer.hpp"

#include "mesh3d/mesh3d.hpp"

enum MESH_DRAW_MODE {
    MESH_DRAW_POINTS,
    MESH_DRAW_LINES,
    MESH_DRAW_LINE_STRIP,
    MESH_DRAW_LINE_LOOP,
    MESH_DRAW_TRIANGLES,
    MESH_DRAW_TRIANGLE_STRIP,
    MESH_DRAW_TRIANGLE_FAN
};


struct gpuAttribBinding {
    const gpuBuffer* buffer;
    int location;
    int count;
    int stride;
    GLenum gl_type;
    bool normalized;
    bool is_instance_array;
    // Debug
    VFMT::GUID guid;

    gpuAttribBinding& operator=(const gpuAttribBinding& other) {
        buffer = other.buffer;
        location = other.location;
        count = other.count;
        stride = other.stride;
        gl_type = other.gl_type;
        normalized = other.normalized;
        is_instance_array = other.is_instance_array;
        guid = other.guid;
        return *this;
    }
};
struct gpuMeshShaderBinding {
    std::vector<gpuAttribBinding> attribs;
    const gpuBuffer* index_buffer;
    int vertex_count;
    int index_count;
    MESH_DRAW_MODE draw_mode;
};

struct gpuMeshMaterialBinding {
    struct BindingData {
        int technique;
        int pass;
        gpuMeshShaderBinding binding;
    };

    std::vector<BindingData> binding_array;
};

class gpuMaterial;
class gpuShaderProgram;
class gpuMeshDesc;
class gpuInstancingDesc;
gpuMeshShaderBinding* gpuCreateMeshShaderBinding(
    const gpuShaderProgram* prog,
    const gpuMeshDesc* desc,
    const gpuInstancingDesc* inst_desc = 0
);
void gpuDestroyMeshShaderBinding(
    gpuMeshShaderBinding* binding
);
bool gpuMakeMeshShaderBinding(
    gpuMeshShaderBinding* out_binding,
    const gpuShaderProgram* prog,
    const gpuMeshDesc* desc,
    const gpuInstancingDesc* inst_desc = 0
);

gpuMeshMaterialBinding* gpuCreateMeshMaterialBinding(
    const gpuMaterial* material,
    const gpuMeshDesc* desc,
    const gpuInstancingDesc* inst_desc = 0
);
void gpuDestroyMeshMaterialBinding(gpuMeshMaterialBinding* binding);
bool gpuMakeMeshMaterialBinding(
    gpuMeshMaterialBinding* binding,
    const gpuMaterial* material,
    const gpuMeshDesc* desc,
    const gpuInstancingDesc* inst_desc = 0
);

class gpuMeshDesc {
public:
    struct AttribDesc {
        VFMT::GUID guid;
        const gpuBuffer* buffer;
        int stride;
    };

    MESH_DRAW_MODE draw_mode = MESH_DRAW_TRIANGLES;
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
    void clear() {
        vertex_count = 0;
        index_count = 0;
        index_array = 0;
        attribs.clear();
    }

    void setVertexCount(int vertex_count) {
        this->vertex_count = vertex_count;
    }
    void setAttribArray(VFMT::GUID attrib_guid, const gpuBuffer* buffer, int stride = 0) {
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
            attribs.push_back(desc);
            std::sort(attribs.begin(), attribs.end(), [](const AttribDesc& a, const AttribDesc& b) -> bool {
                return a.guid < b.guid;
            });
        } else {
            AttribDesc& desc = attribs[found_idx];
            desc.guid = attrib_guid;
            desc.buffer = buffer;
            desc.stride = stride;
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

    // TODO: Come up with some gpuRenderable
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
            desc.stride, (void*)0 /* offset */
        );
    }
    void _bindIndexArray() const {
        index_array->bindIndexArray();
    }
    /*
    void _draw() const {
        GLenum mode;
        switch (draw_mode) {
        case MESH_DRAW_POINTS: mode = GL_POINTS; break;
        case MESH_DRAW_LINES: mode = GL_LINES; break;
        case MESH_DRAW_LINE_STRIP: mode = GL_LINE_STRIP; break;
        case MESH_DRAW_LINE_LOOP: mode = GL_LINE_LOOP; break;
        case MESH_DRAW_TRIANGLES: mode = GL_TRIANGLES; break;
        case MESH_DRAW_TRIANGLE_STRIP: mode = GL_TRIANGLE_STRIP; break;
        case MESH_DRAW_TRIANGLE_FAN: mode = GL_TRIANGLE_FAN; break;
        default: assert(false);
        };
        if (hasIndexArray()) {
            glDrawElements(mode, index_count, GL_UNSIGNED_INT, 0);
        } else {
            glDrawArrays(mode, 0, vertex_count);
        }
    }
    void _drawInstanced(int instance_count) const {
        GLenum mode;
        switch (draw_mode) {
        case MESH_DRAW_POINTS: mode = GL_POINTS; break;
        case MESH_DRAW_LINES: mode = GL_LINES; break;
        case MESH_DRAW_LINE_STRIP: mode = GL_LINE_STRIP; break;
        case MESH_DRAW_LINE_LOOP: mode = GL_LINE_LOOP; break;
        case MESH_DRAW_TRIANGLES: mode = GL_TRIANGLES; break;
        case MESH_DRAW_TRIANGLE_STRIP: mode = GL_TRIANGLE_STRIP; break;
        case MESH_DRAW_TRIANGLE_FAN: mode = GL_TRIANGLE_FAN; break;
        default: assert(false);
        };
        if (hasIndexArray()) {
            glDrawElementsInstanced(mode, index_count, GL_UNSIGNED_INT, 0, instance_count);
        } else {
            glDrawArraysInstanced(mode, 0, vertex_count, instance_count);
        }
    }*/

    void _drawArrays() const {
        glDrawArrays(GL_TRIANGLES, 0, vertex_count);
    }
    void _drawIndexed() const {
        glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
    }

    void _drawArraysLine() const {
        glDrawArrays(GL_LINES, 0, vertex_count);
    }
    void _drawArraysLineStrip() const {
        glDrawArrays(GL_LINE_STRIP, 0, vertex_count);
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
};


#endif
