#pragma once

#include <assert.h>
#include <algorithm>
#include <vector>


enum ANIM_HITBOX_TYPE {
    ANIM_HITBOX_SPHERE,
    ANIM_HITBOX_BOX,
    ANIM_HITBOX_CAPSULE,
    ANIM_HITBOX_LINE
};
struct animHitboxCmd {
    ANIM_HITBOX_TYPE type;
    // TODO: etc.
};
class animHitboxBuffer {
    std::vector<animHitboxCmd> cmds;
    int active_count = 0;
public:
    void reserve(size_t count) { cmds.resize(count); }
    void push(ANIM_HITBOX_TYPE t) {
        if (active_count == cmds.size()) {
            assert(false);
            return;
        }
        cmds[active_count] = animHitboxCmd{ t };
        ++active_count;
    }
    void clear() { active_count = 0; }
    size_t hitboxCount() { return active_count; }
    const animHitboxCmd& operator[](int i) const { return cmds[i]; }
    animHitboxCmd* data() { return &cmds[0]; }
};

class animHitboxSequence {
    struct Block {
        int frame;
        int length;
        ANIM_HITBOX_TYPE type;
    };
    std::vector<Block> blocks;
public:
    void clear() { blocks.clear(); }
    size_t blockCount() const { return blocks.size(); }

    void insert(int frame, int length, ANIM_HITBOX_TYPE type) {
        blocks.push_back(Block{ frame, length, type });
        std::sort(blocks.begin(), blocks.end(), [](const Block& a, const Block& b)->bool {
            return a.frame < b.frame;
        });
    }

    void at(float cursor, int* out_left, int* out_right) {
        int left = 0;
        int right = blocks.size() - 1;
        if(cursor >= (float)blocks.back().frame) {
            left = blocks.size() - 1;
            right = -1;
        } else if(cursor <= blocks.front().frame) {
            left = -1;
            right = 0;
        } else {
            while(right - left > 1) {
                int center = left + (right - left) / 2;
                if(cursor < (float)blocks[center].frame) {
                    right = center;
                } else {
                    left = center;
                }
            }
        }
        *out_left = left;
        *out_right = right;
    }
    void sample(animHitboxBuffer* buf, float cur_min, float cur_max) {
        if (blocks.empty()) {
            return;
        }

        int left = -1;
        int right = -1;
        at(cur_max, &left, &right);
        if (left == -1) {
            return;
        }
        for (int i = 0; i < left + 1; ++i) {
            if (cur_max < (blocks[i].frame + blocks[i].length)) {
                buf->push(blocks[i].type);
            }
        }
    }
};