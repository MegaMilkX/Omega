#include "animator.hpp"

#include "animator_instance.hpp"


HSHARED<AnimatorInstance> AnimatorMaster::createInstance() {
    HSHARED<AnimatorInstance> inst;
    inst.reset_acquire();

    // TODO: Check that skeleton instance matches skeleton prototype

    inst->animator = this;
    
    for (auto& kv : param_names) {
        inst->parameters[kv.second] = .0f;
    }
    for (auto& kv : signal_names) {
        inst->signals[kv.second] = false;
    }
    for (auto& kv : feedback_event_names) {
        inst->feedback_events[kv.second] = false;
    }
    
    inst->samples.init(skeleton.get());

    inst->samplers.resize(samplers.size());
    for (int i = 0; i < samplers.size(); ++i) {
        inst->samplers[i].sampler = animSampler(skeleton.get(), samplers[i].sequence.get());
        inst->samplers[i].samples.init(skeleton.get());
        inst->samplers[i].setSequence(samplers[i].sequence);
        inst->samplers[i].compile(skeleton.get());
        static int next_sync_grp_id = 1;
        if (inst->sync_groups[samplers[i].sync_group] == nullptr) {
            auto pgrp = new animAnimatorSyncGroup;
            inst->sync_groups[samplers[i].sync_group].reset(pgrp);
        }
        inst->sync_groups[samplers[i].sync_group]->addSampler(&inst->samplers[i]);
    }

    int hitbox_cmd_buf_max_len = 0;
    for (auto& kv : inst->sync_groups) {
        kv.second->compile(skeleton.get());
        if (kv.second->hasHitboxSequences()) {
            int max_hitbox_tracks = 0;
            for (int i = 0; i < kv.second->samplerCount(); ++i) {
                auto hit_seq = kv.second->operator[](i).getSequence()->getHitboxSequence();
                if (!hit_seq) {
                    continue;
                }
                if (max_hitbox_tracks < hit_seq->tracks.size()) {
                    max_hitbox_tracks = hit_seq->tracks.size();
                }
            }
            hitbox_cmd_buf_max_len = gfxm::_max(hitbox_cmd_buf_max_len, max_hitbox_tracks);
            inst->sync_groups_hitbox.push_back(kv.second.get());
        }
        if (kv.second->hasAudioSequences()) {
            inst->sync_groups_audio.push_back(kv.second.get());
        }
    }
    inst->hitbox_buffer.resize(hitbox_cmd_buf_max_len);
    inst->audio_cmd_buffer.reserve(8);
    
    instances.insert(inst);

    return inst;
}