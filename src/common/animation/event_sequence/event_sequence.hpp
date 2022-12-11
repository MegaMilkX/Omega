#pragma once

#include <assert.h>
#include <algorithm>
#include <vector>


struct animEventCmd {
    int event;
};
class animEventBuffer {
    std::vector<animEventCmd> cmds;
    int active_count = 0;
public:
    void reserve(size_t count) { cmds.resize(count); }
    void push(int event) {
        if (active_count == cmds.size()) {
            assert(false);
            return;
        }
        cmds[active_count] = animEventCmd{ event };
        ++active_count;
    }
    void clear() { active_count = 0; }
    size_t eventCount() { return active_count; }
    const animEventCmd& operator[](int i) const { return cmds[i]; }
    animEventCmd* data() { return &cmds[0]; }
};

class animEventSequence {
    struct Keyframe {
        int frame;
        int event;
    };
    std::vector<Keyframe> timeline;
public:
    void clear() {
        timeline.clear();
    }

    size_t eventCount() const {
        return timeline.size();
    }

    void insert(int frame, int event) {
        timeline.push_back(Keyframe{ frame, event });
        std::sort(timeline.begin(), timeline.end(), [](const Keyframe& a, const Keyframe& b)->bool {
            return a.frame < b.frame;
        });
    }
    void erase(int frame) {
        for (int i = 0; i < timeline.size(); ++i) {
            if (timeline[i].frame == frame) {
                timeline.erase(timeline.begin() + i);
                break;
            }
        }
    }

    void at(float cursor, int* out_left, int* out_right) {
        if (timeline.empty()) {
            return;
        }

        int left = 0;
        int right = timeline.size() - 1;
        if(cursor >= (float)timeline.back().frame) {
            left = timeline.size() - 1;
            right = timeline.size();
        } else if(cursor <= timeline.front().frame) {
            left = -1;
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
        *out_left = left + timeline.size();
        *out_right = right + timeline.size();
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
    void sample(animEventBuffer* buf, float cur_min, float cur_max, float anim_length) {
        if (timeline.empty()) {
            return;
        }
        if (cur_min == cur_max) {
            return;
        }
        if (cur_min > anim_length) {
            cur_min -= anim_length;
        }
        if (cur_max > anim_length) {
            cur_max -= anim_length;
        }

        int min_left, min_right;
        at(cur_min, &min_left, &min_right);
        int max_left, max_right;
        at(cur_max, &max_left, &max_right);
        int begin = min_right;
        int end = max_right;/*
        if (cur_max < cur_min) {
            last += timeline.size();
        }*/
        begin += timeline.size();
        end += timeline.size();
        if (cur_min > cur_max) {
            end += timeline.size();
        }
        
        for (int i = begin; i < end; ++i) {
            int idx = i % (int)timeline.size();
            int event = timeline[idx].event;
            buf->push(event);
        }
    }
};