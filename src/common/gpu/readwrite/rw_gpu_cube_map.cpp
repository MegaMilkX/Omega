#include "rw_gpu_cube_map.hpp"

#include "base64/base64.hpp"


bool readGpuCubeMapJson(nlohmann::json& json, gpuCubeMap* texture) {
    if (!json.is_string()) {
        assert(false);
        return false;
    }

    std::string base64_str = json.get<std::string>();
    std::vector<char> bytes;
    base64_decode(base64_str.data(), base64_str.size(), bytes);

    return readGpuCubeMapBytes(bytes.data(), bytes.size(), texture);
}
bool writeGpuCubeMapJson(nlohmann::json& json, gpuCubeMap* texture) {
    std::vector<unsigned char> bytes;
    bool ret = writeGpuCubeMapBytes(bytes, texture);
    if (!ret) {
        assert(false);
        return false;
    }
    std::string base64_str;
    base64_encode(bytes.data(), bytes.size(), base64_str);
    json = base64_str;
    return true;
}

bool readGpuCubeMapBytes(const void* data, size_t sz, gpuCubeMap* texture) {
    ktImage img;
    bool ret = loadImage(&img, data, sz);
    if (!ret) {
        assert(false);
        return false;
    }
    texture->setData(&img);
    return true;
}
bool writeGpuCubeMapBytes(std::vector<unsigned char>& out, gpuCubeMap* texture) {
    assert(false);
    // TODO: not implemented
    /*
    ktImage img;
    texture->getData(&img);
    writeImagePng(out, &img);*/
    return false;
}
