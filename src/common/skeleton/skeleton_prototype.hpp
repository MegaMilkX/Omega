#pragma once

#include <set>
#include <vector>
#include <unordered_map>
#include "math/gfxm.hpp"

class sklSkeletonMaster;
class sklSkeletonInstance;

class sklSkeletonPrototype {
    friend sklSkeletonInstance;
    friend sklSkeletonMaster;

    std::vector<int>                        parents;
    std::unordered_map<std::string, int>    name_to_index;
    std::vector<gfxm::mat4>                 default_pose;

public:
    void instantiate(sklSkeletonInstance* inst) const;


    static void reflect();
};
