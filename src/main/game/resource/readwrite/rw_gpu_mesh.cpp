#include "rw_gpu_mesh.hpp"

#include "base64/base64.hpp"

bool readGpuMeshJson(nlohmann::json& json, gpuMesh* mesh) {
    if (!json.is_string()) {
        assert(false);
        return false;
    }

    std::string base64_str = json.get<std::string>();
    std::vector<char> bytes;
    base64_decode(base64_str.data(), base64_str.size(), bytes);

    return readGpuMeshBytes(bytes.data(), bytes.size(), mesh);
}
bool writeGpuMeshJson(nlohmann::json& json, gpuMesh* mesh) {
    std::vector<unsigned char> bytes;
    bool ret = writeGpuMeshBytes(bytes, mesh);
    if (!ret) {
        assert(false);
        return false;
    }
    std::string base64_str;
    base64_encode(bytes.data(), bytes.size(), base64_str);
    json = base64_str;
    return true;
}

bool readGpuMeshBytes(const void* data, size_t sz, gpuMesh* mesh) {
    Mesh3d msh;
    msh.deserialize(data, sz);
    mesh->setData(&msh);
    return true;
}
bool writeGpuMeshBytes(std::vector<unsigned char>& out, gpuMesh* mesh) {
    Mesh3d msh;
    mesh->getMeshDesc()->toMesh3d(&msh);
    msh.serialize(out);
    return true;
}
