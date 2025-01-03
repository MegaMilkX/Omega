#pragma once

#include <assert.h>
#include <map>
#include "resource/resource.hpp"
#include "uaf/uaf.hpp"
#include "math/gfxm.hpp"
#include "curve.hpp"
#include "log/log.hpp"
#include "animation/hitbox_sequence/hitbox_sequence.hpp"
#include "animation/audio_sequence/audio_sequence.hpp"
#include "animation/event_sequence/event_sequence.hpp"
#include "animation/hitbox_sequence_2/hitbox_sequence.hpp"

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

class Animation : public IImportedAsset, public IRuntimeAsset {
    std::vector<AnimNode> nodes;
    std::map<std::string, int> node_name_to_index;
    AnimNode root_motion_node;
    bool has_root_motion = false;

    RHSHARED<hitboxCmdSequence> hitbox_sequence;
    RHSHARED<audioSequence>     audio_sequence;
public:
    TYPE_ENABLE();

    float length = .0f;
    float fps = 60.0f;

    void                                setHitboxSequence(const RHSHARED<hitboxCmdSequence>& hitbox_sequence) { this->hitbox_sequence = hitbox_sequence; }
    const RHSHARED<hitboxCmdSequence>&  getHitboxSequence() const { return hitbox_sequence; }
    RHSHARED<hitboxCmdSequence>         getHitboxSequence() { return hitbox_sequence; }

    void                                setAudioSequence(const RHSHARED<audioSequence>& audio_seq) { this->audio_sequence = audio_seq; }
    const RHSHARED<audioSequence>&      getAudioSequence() const { return audio_sequence; }
    RHSHARED<audioSequence>             getAudioSequence() { return audio_sequence; }

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

    AnimNode* getNode(int index) {
        return &nodes[index];
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

    void sample_root_motion_no_wrap(AnimSample* sample, float from, float to) {
        assert(has_root_motion);
        assert(from <= to);

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
        sample->r = gfxm::inverse(a.r) * b.r;
        sample->s = b.s - a.s;
    }
    void sample_root_motion(AnimSample* sample, float from, float to) {
        assert(has_root_motion);
        
        if (fabs(to - from) <= FLT_EPSILON) {
            sample->t = gfxm::vec3(0, 0, 0);
            sample->r = gfxm::quat(0, 0, 0, 1);
            sample->s = gfxm::vec3(0, 0, 0);
        } else if (to < from) {
            AnimSample aa;
            sample_root_motion_no_wrap(&aa, .0f, to);
            AnimSample bb;
            sample_root_motion_no_wrap(&bb, gfxm::_min(from, length), length);
            sample->t = aa.t + bb.t;
            sample->r = bb.r * aa.r;
            sample->s = aa.s + bb.s;
        } else {
            sample_root_motion_no_wrap(sample, from, to);
        }
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
            if (out_index == -1) {
                continue;
            }
            assert(out_index >= 0 && out_index < sample_count);
            AnimSample& result = samples[out_index];
            result.t = n.t.at(cursor);
            result.r = n.r.at(cursor);
            result.s = n.s.at(cursor);
        }
    }
    void sample(
        AnimSample* samples,
        int sample_count,
        float cursor
    ) {
        for(size_t i = 0; i < nodes.size() && i < sample_count; ++i) {
            auto& n = nodes[i];
            int32_t out_index = i;
            assert(out_index >= 0 && out_index < sample_count);
            AnimSample& result = samples[out_index];
            result.t = n.t.at(cursor);
            result.r = n.r.at(cursor);
            result.s = n.s.at(cursor);
        }
    }

    bool serialize(std::vector<unsigned char>& buf) const;
    bool deserialize(const void* data, size_t sz);
    void serializeJson(nlohmann::json& json) const override;
    bool deserializeJson(const nlohmann::json& json) override;
};
inline RHSHARED<Animation> getAnimation(const char* path) {
    return resGet<Animation>(path);
}


bool animInit();
void animCleanup();