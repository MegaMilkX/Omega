#pragma once

#include <stdint.h>
#include <vector>
#include "csg_types.hpp"
#include "csg_vertex.hpp"


struct csgFace;
struct csgFragment {
    csgFace* face = 0;
    CSG_RELATION_TYPE rel_type;
    CSG_VOLUME_TYPE front_volume;
    CSG_VOLUME_TYPE back_volume;
    uint32_t rgba = 0xFFFFFFFF;
    gfxm::vec3 N;
    std::vector<csgVertex> vertices;
};
