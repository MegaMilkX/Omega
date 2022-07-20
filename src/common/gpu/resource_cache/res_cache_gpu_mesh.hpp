#pragma once


#include "resource/res_cache_interface.hpp"

#include "gpu/gpu_mesh.hpp"

#include "gpu/readwrite/rw_gpu_mesh.hpp"


class resCacheGpuMesh : public resCacheInterface {
    std::map<std::string, HSHARED<gpuMesh>> meshes;

    bool loadMeshBytes(gpuMesh* mesh, const char* path) {
        FILE* f = fopen(path, "rb");
        if (!f) {
            return false;
        }
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);
        std::vector<unsigned char> bytes(fsize, 0);
        fread(&bytes[0], fsize, 1, f);
        fclose(f);

        return readGpuMeshBytes(&bytes[0], bytes.size(), mesh);
    }

public:
    resCacheGpuMesh() {
        
    }
    HSHARED_BASE* get(const char* name) override {
        auto it = meshes.find(name);
        if (it == meshes.end()) {
            Handle<gpuMesh> handle = HANDLE_MGR<gpuMesh>::acquire();
            if (!loadMeshBytes(HANDLE_MGR<gpuMesh>::deref(handle), name)) {
                HANDLE_MGR<gpuMesh>::release(handle);
                return 0;
            }
            it = meshes.insert(std::make_pair(std::string(name), HSHARED<gpuMesh>(handle))).first;
            it->second.setReferenceName(name);
        }
        return &it->second;
    }
};
