#pragma once

#include "math/gfxm.hpp"


struct hitboxSeqSample {
    HITBOX_SEQ_CLIP_TYPE type;
    gfxm::vec3  translation;
    float       radius;

    gfxm::mat4 parent_transform = gfxm::mat4(1.0f);
};