#pragma once

#include "actor_node_link.hpp"
#include "skeleton/skeleton_editable.hpp"


struct SkeletonNodeLink : public ActorNodeLink {
    HSHARED<SkeletonInstance> skeleton_instance;
};

