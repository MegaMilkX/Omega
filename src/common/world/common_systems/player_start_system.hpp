#pragma once

#include <vector>
#include "math/gfxm.hpp"


struct PlayerStartSystem {
    struct Location {
        gfxm::vec3 origin;
        gfxm::vec3 rotation;
    };

    std::vector<Location> points;
};