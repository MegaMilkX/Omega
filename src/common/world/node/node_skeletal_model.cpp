#include "node_skeletal_model.hpp"


STATIC_BLOCK {
    type_register<SkeletalModelNode>("SkeletalModelNode")
        .parent<ActorNode>();
};