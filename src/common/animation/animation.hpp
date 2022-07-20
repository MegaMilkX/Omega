#pragma once

#include <assert.h>
#include <map>
#include "math/gfxm.hpp"
#include "curve.hpp"

struct AnimNode {
    curve<gfxm::vec3> t;
    curve<gfxm::quat> r;
    curve<gfxm::vec3> s;
};

struct AnimSample {
    gfxm::vec3 t;
    gfxm::quat r;
    gfxm::vec3 s;
};

class Animation {
    std::vector<AnimNode> nodes;
    std::map<std::string, int> node_name_to_index;
    AnimNode root_motion_node;
    bool has_root_motion = false;
public:
    float length = .0f;
    float fps = 60.0f;

    AnimNode& createNode(const std::string& name) {
        assert(node_name_to_index.find(name) == node_name_to_index.end());
        node_name_to_index[name] = nodes.size();
        nodes.emplace_back(AnimNode());
        return nodes.back();
    }

    void addRootMotionNode(const AnimNode& node) {
        root_motion_node = node;
        has_root_motion = true;
    }

    AnimNode* getNode(const std::string& name) {
        int idx = getNodeIndex(name);
        if (idx < 0) {
            return 0;
        }
        return &nodes[idx];
    }

    int getNodeIndex(const std::string& name) const {
        auto it = node_name_to_index.find(name);
        if(it == node_name_to_index.end()) {
            return -1;
        }
        return it->second;
    }

    bool hasRootMotion() const {
        return has_root_motion;
    }

    size_t nodeCount() const {
        return nodes.size();
    }

    void sample_root_motion(AnimSample* sample, float from, float to) {
        assert(has_root_motion);
        auto& n = root_motion_node;
        AnimSample a;
        a.t = n.t.at(from);
        a.r = n.r.at(from);
        a.s = n.s.at(from);
        AnimSample b;
        b.t = n.t.at(to);
        b.r = n.r.at(to);
        b.s = n.s.at(to);

        sample->t = b.t - a.t;
        sample->r = gfxm::inverse(b.r) * a.r;
        sample->s = b.s - a.s;
    }
    void sample_remapped(
        AnimSample* samples,
        int sample_count,
        float cursor,
        const std::vector<int32_t>& mapping
    ) {
        for(size_t i = 0; i < nodes.size() && i < sample_count; ++i) {
            auto& n = nodes[i];
            int32_t out_index = mapping[i];
            assert(out_index >= 0 && out_index < sample_count);
            AnimSample& result = samples[out_index];
            result.t = n.t.at(cursor);
            result.r = n.r.at(cursor);
            result.s = n.s.at(cursor);
        }
    }

    bool serialize(std::vector<unsigned char>& buf);
    bool deserialize(const void* data, size_t sz);
};


bool animInit();
void animCleanup();