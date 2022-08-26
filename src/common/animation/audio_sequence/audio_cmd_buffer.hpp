#pragma once

#include "skeleton/skeleton_instance.hpp"
#include "audio/audio_mixer.hpp"
#include "audio/audio_clip.hpp"
#include "handle/hshared.hpp"

struct audioCmd {
    int bone_id = 0;
    float gain = 1.0f;
    RHSHARED<AudioClip> clip;
};

class audioCmdBuffer {
    std::vector<audioCmd> cmds;
    int active_cmd_count = 0;
public:
    void reserve(size_t count) { cmds.resize(count); }
    
    void pushCmd(const audioCmd& cmd) {
        if (active_cmd_count == cmds.size()) {
            assert(false);
            return;
        }
        cmds[active_cmd_count] = cmd;
        active_cmd_count++;
    }
    
    void clearActive() { active_cmd_count = 0; }
    size_t activeCount() { return active_cmd_count; }
    audioCmd& operator[](int i) { return cmds[i]; }
    const audioCmd& operator[](int i) const { return cmds[i]; }
    audioCmd* data() { return &cmds[0]; }

    void execute(sklSkeletonInstance* skl_inst) {
        for (int i = 0; i < active_cmd_count; ++i) {
            auto& cmd = cmds[i];
            const gfxm::mat4& m = skl_inst->getWorldTransformsPtr()[cmd.bone_id];
            const gfxm::vec3 trans = m * gfxm::vec4(0, 0, 0, 1);
            audio().playOnce3d(cmd.clip->getBuffer(), trans, cmd.gain);
        }
    }
};