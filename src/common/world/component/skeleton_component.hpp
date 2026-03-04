#pragma once

#include "skeleton_component.auto.hpp"
#include "actor_component.hpp"
#include "skeleton/skeleton_editable.hpp"


[[cppi_class]];
class SkeletonComponent : public ActorComponent {
    ResourceRef<Skeleton> skeleton;
public:
    TYPE_ENABLE();

    [[cppi_decl, set("skeleton")]]
    void setSkeleton(ResourceRef<Skeleton> skl) {
        skeleton = skl;
        // TODO: Update dependents somehow
    }
    [[cppi_decl, get("skeleton")]]
    ResourceRef<Skeleton> getSkeleton() const {
        return skeleton;
    }
};