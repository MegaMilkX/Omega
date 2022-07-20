#include "animation.hpp"

#include "reflection/reflection.hpp"


#include "serialization/virtual_obuf.hpp"
#include "serialization/virtual_ibuf.hpp"
bool Animation::serialize(std::vector<unsigned char>& buf) {
    const uint32_t tag = *(uint32_t*)"SANM";
    const uint32_t version = 0;

    vofbuf vof;

    vof.write<uint32_t>(tag);
    vof.write<uint32_t>(version);

    vof.write<float>(length);
    vof.write<float>(fps);

    vof.write<uint32_t>(nodes.size());
    for (int i = 0; i < nodes.size(); ++i) {
        auto& node = nodes[i];
        auto& t_curve = node.t;
        auto& r_curve = node.r;
        auto& s_curve = node.s;

        std::vector<curve<gfxm::vec3>::keyframe_t>& t_keyframes = t_curve.get_keyframes();
        std::vector<curve<gfxm::quat>::keyframe_t>& r_keyframes = r_curve.get_keyframes();
        std::vector<curve<gfxm::vec3>::keyframe_t>& s_keyframes = s_curve.get_keyframes();

        vof.write_vector(t_keyframes, true);
        vof.write_vector(r_keyframes, true);
        vof.write_vector(s_keyframes, true);
    }
    vof.write<uint32_t>(node_name_to_index.size());
    for (auto& kv : node_name_to_index) {
        vof.write_string(kv.first);
        vof.write<int32_t>(kv.second);
    }

    buf.insert(buf.end(), vof.getData(), vof.getData() + vof.getSize());
    return true;
}
bool Animation::deserialize(const void* data, size_t sz) {
    vifbuf vif((unsigned char*)data, sz);

    uint32_t tag = vif.read<uint32_t>();
    uint32_t version = vif.read<uint32_t>();

    length = vif.read<float>();
    fps = vif.read<float>();

    uint32_t node_count = vif.read<uint32_t>();
    nodes.resize(node_count);
    for (int i = 0; i < node_count; ++i) {
        auto& node = nodes[i];
        auto& t_curve = node.t;
        auto& r_curve = node.r;
        auto& s_curve = node.s;

        std::vector<curve<gfxm::vec3>::keyframe_t> t_keyframes;
        std::vector<curve<gfxm::quat>::keyframe_t> r_keyframes;
        std::vector<curve<gfxm::vec3>::keyframe_t> s_keyframes;

        vif.read_vector(t_keyframes);
        vif.read_vector(r_keyframes);
        vif.read_vector(s_keyframes);

        t_curve.set_keyframes(t_keyframes);
        r_curve.set_keyframes(r_keyframes);
        s_curve.set_keyframes(s_keyframes);
    }
    uint32_t name_count = vif.read<uint32_t>();
    for (int i = 0; i < name_count; ++i) {
        std::string name = vif.read_string();
        int32_t index = vif.read<int32_t>();
        node_name_to_index[name] = index;
    }

    return true;
}


#include "resource/resource.hpp"
#include "animation/resource_cache/res_cache_animation.hpp"

bool animInit() {
    type_register<Animation>("Animation");
    
    resAddCache<Animation>(new resCacheAnimation);

    return true;
}
void animCleanup() {

}
