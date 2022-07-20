#include "rw_animation.hpp"

#include "base64/base64.hpp"

bool readAnimationJson(nlohmann::json& json, Animation* anim) {
    if (!json.is_string()) {
        assert(false);
        return false;
    }

    std::string base64_str = json.get<std::string>();
    std::vector<char> bytes;
    base64_decode(base64_str.data(), base64_str.size(), bytes);

    return readAnimationBytes(bytes.data(), bytes.size(), anim);
}
bool writeAnimationJson(nlohmann::json& json, Animation* anim) {
    std::vector<unsigned char> bytes;
    bool ret = writeAnimationBytes(bytes, anim);
    if (!ret) {
        assert(false);
        return false;
    }
    std::string base64_str;
    base64_encode(bytes.data(), bytes.size(), base64_str);
    json = base64_str;
    return true;
}

bool readAnimationBytes(const void* data, size_t sz, Animation* anim) {
    return anim->deserialize(data, sz);
}
bool writeAnimationBytes(std::vector<unsigned char>& out, Animation* anim) {
    return anim->serialize(out);
}
