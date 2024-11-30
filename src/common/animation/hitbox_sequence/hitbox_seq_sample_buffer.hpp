#pragma once

#include "animation/hitbox_sequence/hitbox_sequence.hpp"

#include "skeleton/skeleton_instance.hpp"
#include "collision/collision_world.hpp"

struct hitboxCmdBuffer {
    std::vector<hitboxCmd> samples;
    int active_sample_count = 0;

    void resize(size_t count) {
        samples.resize(count);
    }

    void clearActive() {
        active_sample_count = 0;
    }

    size_t count() const { return samples.size(); }
    hitboxCmd& operator[](int index) { return samples[index]; }
    const hitboxCmd& operator[](int index) const { return samples[index]; }
    hitboxCmd* data() { return &samples[0]; }
    const hitboxCmd* data() const { return &samples[0]; }

    void execute(SkeletonInstance* skl_inst, CollisionWorld* col_wrld) {
        for (int i = 0; i < active_sample_count; ++i) {
            auto& s = samples[i];
            if (s.type == HITBOX_SEQ_CLIP_EMPTY) {
                continue;
            }
            gfxm::vec3 trans
                = skl_inst->getBoneNode(s.bone_id)->getWorldTransform()
                * gfxm::vec4(s.translation, 1.0f);
            gfxm::mat4 m = gfxm::translate(gfxm::mat4(1.0f), trans);
            col_wrld->sphereTest(m, s.radius);
        }
    }
};