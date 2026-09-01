#include "rw_gpu_cube_map.hpp"

#include "base64/base64.hpp"


bool readGpuCubeMapJson(const nlohmann::json& json, gpuCubeTexture* texture) {
    if (!json.is_string()) {
        assert(false);
        return false;
    }

    std::string base64_str = json.get<std::string>();
    std::vector<char> bytes;
    base64_decode(base64_str.data(), base64_str.size(), bytes);

    return readGpuCubeMapBytes(bytes.data(), bytes.size(), texture);
}
bool writeGpuCubeMapJson(nlohmann::json& json, gpuCubeTexture* texture) {
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

bool readGpuCubeMapBytes(const void* data, size_t sz, gpuCubeTexture* texture) {
    ktImage img;
    bool ret = loadImage(&img, data, sz);
    if (!ret) {
        assert(false);
        return false;
    }
    texture->setData(&img);
    return true;
}
bool writeGpuCubeMapBytes(std::vector<unsigned char>& out, gpuCubeTexture* texture) {
    assert(false);
    // TODO: not implemented
    /*
    ktImage img;
    texture->getData(&img);
    writeImagePng(out, &img);*/
    return false;
}
