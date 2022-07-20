#include "rw_gpu_texture_2d.hpp"

#include "base64/base64.hpp"

bool readGpuTexture2dJson(nlohmann::json& json, gpuTexture2d* texture) {
    if (!json.is_string()) {
        assert(false);
        return false;
    }

    std::string base64_str = json.get<std::string>();
    std::vector<char> bytes;
    base64_decode(base64_str.data(), base64_str.size(), bytes);

    return readGpuTexture2dBytes(bytes.data(), bytes.size(), texture);
}
bool writeGpuTexture2dJson(nlohmann::json& json, gpuTexture2d* texture) {
    std::vector<unsigned char> bytes;
    bool ret = writeGpuTexture2dBytes(bytes, texture);
    if (!ret) {
        assert(false);
        return false;
    }
    std::string base64_str;
    base64_encode(bytes.data(), bytes.size(), base64_str);
    json = base64_str;
    return true;
}

#include "image/image.hpp"
bool readGpuTexture2dBytes(const void* data, size_t sz, gpuTexture2d* texture) {
    ktImage img;
    bool ret = loadImage(&img, data, sz);
    if (!ret) {
        assert(false);
        return false;
    }
    texture->setData(&img);
    return true;
}
bool writeGpuTexture2dBytes(std::vector<unsigned char>& out, gpuTexture2d* texture) {
    ktImage img;
    texture->getData(&img);
    writeImagePng(out, &img);
    return true;
}