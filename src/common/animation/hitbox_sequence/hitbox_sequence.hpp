#pragma once

#include <vector>
#include "skeleton/skeleton_editable.hpp"
#include "animation/hitbox_sequence/hitbox_seq_track.hpp"
#include "animation/hitbox_sequence/hitbox_seq_sample.hpp"

struct hitboxCmdSequence {
    RHSHARED<Skeleton> skeleton;
    std::vector<std::unique_ptr<hitboxSeqTrack>> tracks;
    float fps = 60.0f;
    float length = .0f;

    hitboxSeqTrack* addTrack() {
        hitboxSeqTrack* track = new hitboxSeqTrack;
        tracks.push_back(std::unique_ptr<hitboxSeqTrack>(track));
        return track;
    }

    int sample(hitboxCmd* samples, size_t sample_count, float cursor_from, float cursor_to) {
        if (tracks.size() > sample_count) {
            assert(false);
        }
        int total_samples = 0;
        for (int i = 0; i < tracks.size() && i < sample_count; ++i) {
            auto t = tracks[i].get();
            float clip_cursor = .0f;
            float clip_cursor_prev = .0f;
            if (t->timeline.empty()) {
                continue;
            }
            auto clip_prev = t->at(cursor_from, clip_cursor_prev);
            auto clip = t->at(cursor_to, clip_cursor);
            if (!clip) {
                continue;
            }
            if (clip_cursor > clip->length || clip_cursor < .0f) {
                continue;
            }
            if (clip->bone_id < 0) {
                assert(false);
                continue;
            }
            if ((clip != clip_prev || cursor_from >= cursor_to) || clip->keep_following_bone) {
                // TODO: Remove this block
                //samples[i].parent_transform = skl_inst->getWorldTransformsPtr()[clip->bone_id];
            }
            samples[total_samples].bone_id = clip->bone_id;
            samples[total_samples].type = HITBOX_SEQ_CLIP_SPHERE;
            samples[total_samples].translation = clip->translation.at(clip_cursor, gfxm::vec3(0,0,0));
            samples[total_samples].radius = clip->radius.at(clip_cursor, .5f);
            total_samples++;
        }
        return total_samples;
    }
};