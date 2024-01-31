#include "animator_instance.hpp"

#include "animator.hpp"


Skeleton* AnimatorInstance::getSkeletonMaster() { 
    return animator->getSkeleton();
}

void AnimatorInstance::update(float dt) {
    auto rootUnit = animator->getRoot();
    if (!rootUnit) {
        return;
    }

    // Clear feedback events
    for (auto& kv : feedback_events) {
        kv.second = false;
    }

    // Clear sampler influence weights
    for (auto& sg : sync_groups) {
        sg.second->clearInfluence();
    }
    // Calc new influence weights
    rootUnit->updateInfluence(this, 1.0f);
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
    for (auto& sg : sync_groups_audio) {
        auto sampler = sg->getTopLevelSampler();
        if (sampler->total_influence == .0f) {
            continue;
        }
        auto audio_seq = sampler->getSequence()->getAudioSequence();
        float cur_n_prev = sampler->cursor_prev / sampler->length_scaled;
        float cur_n = sampler->cursor / sampler->length_scaled;
        audio_seq->sample(&audio_cmd_buffer, cur_n_prev * audio_seq->length, cur_n * audio_seq->length);
    }

    // Clear signals
    for (auto& kv : signals) {
        kv.second = false;
    }
}