#ifndef GPU_MESH_DESC_HPP
#define GPU_MESH_DESC_HPP

#include <algorithm>
#include <vector>

#include <assert.h>
#include "vertex_format.hpp"
#include "gpu_buffer.hpp"

enum MESH_DRAW_MODE {
    MESH_DRAW_POINTS,
    MESH_DRAW_LINES,
    MESH_DRAW_LINE_STRIP,
    MESH_DRAW_LINE_LOOP,
    MESH_DRAW_TRIANGLES,
    MESH_DRAW_TRIANGLE_STRIP,
    MESH_DRAW_TRIANGLE_FAN
};

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

    int findAttribDesc(VFMT::GUID guid) const {
        if (attribs.empty()) {
            return -1;
        }
        int begin = 0;
        int end = attribs.size() - 1;
        while (true) {
            int middle = begin + (end - begin) / 2;
            if(guid == attribs[middle].guid) {
                return middle;
            } else if(begin == end) {
                return -1;
            } else if(guid < attribs[middle].guid) {
                end = middle - 1;
            } else if(guid > attribs[middle].guid) {
                begin = middle + 1;
            }
        }
        return -1;
    }

public:
    void setVertexCount(int vertex_count) {
        this->vertex_count = vertex_count;
    }
    void setAttribArray(VFMT::GUID attrib_guid, const gpuBuffer* buffer, int stride = 0) {
        int found_idx = findAttribDesc(attrib_guid);
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
        int id = findAttribDesc(attrib_guid);
        return id;
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

    // TODO: Come up with some gpuRenderable
    void _bindVertexArray(VFMT::GUID attrib_guid, int location) const {
        int id = findAttribDesc(attrib_guid);
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
};

class gpuShaderMeshMapping {};


#endif