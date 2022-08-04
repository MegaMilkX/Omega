#pragma once

#include <vector>
#include "animation/hitbox_sequence/hitbox_seq_type.hpp"
#include "animation/curve.hpp"


struct hitboxSeqClip {
    HITBOX_SEQ_CLIP_TYPE type;
    int                 length;
    int                 bone_id = -1;
    bool                keep_following_bone;
    curve<gfxm::vec3>   translation;
    curve<float>        radius;
};

struct hitboxSeqTrackClipRef {
    int frame = 0;
    hitboxSeqClip* clip = 0;
};

struct hitboxSeqTrack {
    std::vector<hitboxSeqTrackClipRef> timeline;

    void insert(int frame, hitboxSeqClip* clip) {
        timeline.push_back(hitboxSeqTrackClipRef{ frame, clip });
        std::sort(timeline.begin(), timeline.end(), [](const hitboxSeqTrackClipRef& a, const hitboxSeqTrackClipRef& b)->bool {
            return a.frame < b.frame;
        });
    }
    hitboxSeqClip* at(float cursor, float& out_clip_cursor) {
        if (timeline.empty()) {
            out_clip_cursor = .0f;
            return 0;
        }

        int left = 0;
        int right = timeline.size() - 1;
        if(cursor > (float)timeline.back().frame) {
            left = timeline.size() - 1;
            right = left;
            out_clip_cursor = cursor - (float)timeline[left].frame;
        } else if(cursor < 0) {
            left = 0;
            right = 0;
            out_clip_cursor = cursor;
        } else {
            while(right - left > 1) {
                int center = left + (right - left) / 2;
                if(cursor < (float)timeline[center].frame) {
                    right = center;
                } else {
                    left = center;
                }
            }
            out_clip_cursor = cursor - (float)timeline[left].frame;
        }

        hitboxSeqClip* a = timeline[left].clip;
        if (out_clip_cursor < .0f || out_clip_cursor >= a->length) {
            a = nullptr;
        }
        return a;
    }
};