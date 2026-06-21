#include "animator_instance.hpp"

#include "animator.hpp"


Skeleton* AnimMachineInstance::getSkeletonMaster() { 
    return animator->getSkeleton();
}

bool AnimMachineInstance::init(ResourceRef<AnimMachine>& anim_machine) {    
    animator = anim_machine;

    instance_data.fsm_data.resize(anim_machine->compile_context.fsm_count);

    vm_program = anim_machine->vm_program;
    vm.load_program(&vm_program);
    vm.set_host_event_cb(std::bind(&AnimMachineInstance::onHostEventCb, this, std::placeholders::_1));
    
    auto skel = anim_machine->getSkeleton();
    samples.init(skel);

    samplers.resize(anim_machine->samplers.size());
    for (int i = 0; i < samplers.size(); ++i) {
        samplers[i].sampler = animSampler(skel, anim_machine->samplers[i].sequence.get());
        samplers[i].samples.init(skel);
        samplers[i].setSequence(anim_machine->samplers[i].sequence);
        samplers[i].compile(skel);

        if (sync_groups[anim_machine->samplers[i].sync_group] == nullptr) {
            auto pgrp = new animAnimatorSyncGroup;
            sync_groups[anim_machine->samplers[i].sync_group].reset(pgrp);
        }
        sync_groups[anim_machine->samplers[i].sync_group]->addSampler(&samplers[i]);
    }

    int hitbox_cmd_buf_max_len = 0;
    for (auto& kv : sync_groups) {
        kv.second->compile(skel);
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
            sync_groups_hitbox.push_back(kv.second.get());
        }
        if (kv.second->hasAudioSequences()) {
            sync_groups_audio.push_back(kv.second.get());
        }
    }
    hitbox_buffer.resize(hitbox_cmd_buf_max_len);
    audio_cmd_buffer.reserve(8);

    anim_machine->rootUnit->prepareInstance(this);
}

bool AnimMachineInstance::update(float dt) {
    if (!animator) {
        return false;
    }
    auto rootUnit = animator->getRoot();
    if (!rootUnit) {
        return false;
    }

    // Clear feedback events
    for (auto& kv : feedback_events) {
        kv.second = false;
    }
    // Clear state_complete
    vm_program.set_variable_bool(vm_program.find_variable("state_complete").addr, 0);

    // Clear sampler influence weights
    for (auto& sg : sync_groups) {
        sg.second->clearInfluence();
    }
    // Calc new influence weights
    rootUnit->updateInfluence(animator.get(), this, 1.0f);
    // Update sampler length scale based on influence weights
    for (auto& sg : sync_groups) {
        sg.second->updateLengths();
    }
    // Sample animations based on scaled length and influence weights (.0f skips sampling but advances cursor)
    for (auto& sg : sync_groups) {
        sg.second->sampleClips(dt);
    }
    // Update animator tree
    // Actually just blend samples
    rootUnit->update(this, &samples, dt);

    // Hitboxes
    hitbox_buffer.clearActive();
    int hitbox_buf_first = 0;
    for (auto& sg : sync_groups_hitbox) {
        auto sampler = sg->getTopLevelSampler();
        if (sampler->total_influence == .0f) {
            continue;
        }
        auto hitbox_seq = sampler->getSequence()->getHitboxSequence();
        float cur_n_prev = sampler->cursor_prev / sampler->length_scaled;
        float cur_n = sampler->cursor / sampler->length_scaled;
        int sample_count = hitbox_seq->sample(
            &hitbox_buffer[hitbox_buf_first], hitbox_buffer.count() - hitbox_buf_first,
            cur_n_prev * hitbox_seq->length,
            cur_n * hitbox_seq->length
        );
        hitbox_buf_first += sample_count;
        hitbox_buffer.active_sample_count += sample_count;
    }
    // Audio commands
    audio_cmd_buffer.clearActive();
    for (auto& sg : sync_groups) {
        auto sampler = sg.second->getTopLevelSampler();
        if (sampler->total_influence == .0f) {
            continue;
        }
        auto audio_seq = sampler->getSequence()->getAudioSequence();
        if (!audio_seq) {
            continue;
        }
        float cur_n_prev = sampler->cursor_prev / sampler->length_scaled;
        float cur_n = sampler->cursor / sampler->length_scaled;
        audio_seq->sample(&audio_cmd_buffer, cur_n_prev * audio_seq->length, cur_n * audio_seq->length);
    }

    // Clear signals
    for (auto addr : animator->signals) {
        vm_program.set_variable_float(addr, .0f);
    }

    return true;
}