#ifndef GPU_MESH_BUFFER_HPP
#define GPU_MESH_BUFFER_HPP

#include <vector>
#include <memory>
#include "mesh3d/mesh3d.hpp"
#include "gpu_buffer.hpp"
#include "gpu_mesh_desc.hpp"
#include "gpu/vertex_format.hpp"
#include "reflection/reflection.hpp"

// TODO
class gpuVertexBuffers {
    std::vector<std::unique_ptr<gpuBuffer>> buffers;
    gpuMeshDesc                             mesh_desc;
public:
    gpuVertexBuffers() {}
    virtual ~gpuVertexBuffers() {}

    const gpuMeshDesc* getMeshDesc() const { return &mesh_desc; }


};
// TODO


class gpuMesh {
    std::vector<gpuBuffer*> buffers;
    std::unique_ptr<gpuBuffer> index_buffer;

    gpuMeshDesc mesh_desc;

public:
    TYPE_ENABLE();

    gpuMesh() {}
    ~gpuMesh() {
        for(auto b : buffers) {
            delete b;
        }
    }

    const gpuMeshDesc* getMeshDesc() const {
        return &mesh_desc;
    }

    void setData(const Mesh3d* mesh) {
        size_t attrib_count = mesh->getAttribArrayCount();
        for (int i = 0; i < attrib_count; ++i) {
            auto guid = mesh->getAttribArrayGUID(i);
            const void* data = mesh->getAttribArrayData(guid);
            size_t size = mesh->getAttribArraySize(guid);
            setAttribArray(guid, data, size);
        }
        if (mesh->hasIndices()) {
            const void* data = mesh->getIndexArrayData();
            size_t size = mesh->getIndexArraySize();
            setIndexArray(data, size);
        }
    }
    void getData(Mesh3d* mesh) {
        
    }

    void setDrawMode(MESH_DRAW_MODE mode) {
        mesh_desc.draw_mode = mode;
    }

    const gpuBuffer* getIndexBuffer() const {
        return index_buffer.get();
    }

private:
    void setAttribArray(VFMT::GUID attrib_gid, const void* data, size_t size) {
        auto attr = VFMT::getAttribDesc(attrib_gid);
        
        gpuBuffer* buf = new gpuBuffer();
        buf->setArrayData(data, size);

        buffers.push_back(buf);

        mesh_desc.setAttribArray(attrib_gid, buf, 0);
        mesh_desc.setVertexCount(size / (attr->elem_size * attr->count) /* TODO: ??? */);
    }
    void setIndexArray(const void* data, size_t size) {
        index_buffer.reset(new gpuBuffer());
        index_buffer->setArrayData(data, size);
        mesh_desc.setIndexArray(index_buffer.get());
    }
};


#endif
