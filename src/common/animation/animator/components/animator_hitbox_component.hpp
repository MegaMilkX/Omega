#pragma once

#include "animator_component.hpp"
#include "animation/hitbox_sequence/hitbox_seq_sample_buffer.hpp"


class animAnimatorHitboxComponent : public animAnimatorComponent {
public:
    std::unordered_map<std::string, animAnimatorSyncGroup*> relevant_sync_groups;
    hitboxCmdBuffer sample_buf;

    void addSyncGroup(const char* name, animAnimatorSyncGroup* grp) override {
        bool has_hitbox_seq = false;
        for (int i = 0; i < grp->samplerCount(); ++i) {
            auto hitbox_seq = grp->operator[](i).seq->getGenericTimeline<hitboxCmdSequence>();
            if (hitbox_seq) {
                has_hitbox_seq = true;
                relevant_sync_groups.insert(std::make_pair(std::string(name), grp));
                break;
            }
        }
    }
    void onUpdate(animAnimatorSyncGroup* group, float cursor_from, float cursor_to) override {
        for (auto& kv : relevant_sync_groups) {
            auto& sampler = kv.second->getSorted(0); // Top weighted sampler
            auto& seq = sampler.seq;
            auto tl = seq->getGenericTimeline<hitboxCmdSequence>();
            if (!tl) {
                break;
            }
            tl->sample(skl_inst, sample_buf.data(), sample_buf.count(), cursor_from, cursor_to);
        }
    }
};