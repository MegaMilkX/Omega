#pragma once

#include <vector>
#include "audio/audio_mixer.hpp"
#include "audio/audio_clip.hpp"
#include "handle/hshared.hpp"

#include "animation/audio_sequence/audio_cmd_buffer.hpp"

class audioSequence {
    struct Keyframe {
        int frame;
        RHSHARED<AudioClip> clip;
    };

    std::vector<Keyframe> timeline;
public:
    float fps = 60.0f;
    float length = .0f;

    void insert(int frame, const RHSHARED<AudioClip>& clip) {
        timeline.push_back(Keyframe{ frame, clip });
        std::sort(timeline.begin(), timeline.end(), [](const Keyframe& a, const Keyframe& b)->bool {
            return a.frame < b.frame;
        });
    }

    void at(float cursor, int* out_left, int* out_right) {
        if (timeline.empty()) {
            return;
        }

        int left = 0;
        int right = timeline.size() - 1;
        if(cursor >= (float)timeline.back().frame) {
            left = timeline.size() - 1;
            right = timeline.size(); // 0th keyframe, but wrapped around
        } else if(cursor <= timeline.front().frame) {
            left = timeline.size() - 1 + timeline.size();
            right = 0;
        } else {
            while(right - left > 1) {
                int center = left + (right - left) / 2;
                if(cursor < (float)timeline[center].frame) {
                    right = center;
                } else {
                    left = center;
                }
            }
        }

        *out_left = left;
        *out_right = right;
    }
    int at_left(float cursor) {
        int l = 0, r = 0;
        at(cursor, &l, &r);
        return l;
    }
    int at_right(float cursor) {
        int l = 0, r = 0;
        at(cursor, &l, &r);
        return r;
    }


    void sample(audioCmdBuffer* buf, float cur_min, float cur_max) {
        if (cur_min == cur_max) {
            return;
        }
        if (cur_min > length) {
            cur_min -= length;
        }
        if (cur_max > length) {
            cur_max -= length;
        }

        int first = at_right(cur_min);
        int last = at_left(cur_max);
        if (cur_max < cur_min) {
            last += timeline.size();
        }
        
        if (first <= last) {
            for (int i = first; i <= last; ++i) {
                audioCmd cmd;
                cmd.bone_id = 0;
                cmd.clip = timeline[i % timeline.size()].clip;
                cmd.gain = 1.0f;
                buf->pushCmd(cmd);
            }
        }
    }
};