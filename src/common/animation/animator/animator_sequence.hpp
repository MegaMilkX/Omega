#pragma once

#include <unordered_map>
#include "reflection/reflection.hpp"
#include "handle/hshared.hpp"
#include "animation/animation.hpp"


struct animGenericTimelineContainer {
    virtual ~animGenericTimelineContainer() {}
};
template<typename T>
struct animGenericTimelineContainerT : public animGenericTimelineContainer {
    RHSHARED<T> ref;
};
class animAnimatorSequence {
    RHSHARED<Animation> anim;
    std::unordered_map<type, std::unique_ptr<animGenericTimelineContainer>> generic_timelines;
public:
    void setSkeletalAnimation(const RHSHARED<Animation>& anim) { this->anim = anim; }
    const RHSHARED<Animation>& getSkeletalAnimation() const { return anim; }
    RHSHARED<Animation> getSkeletalAnimation() { return anim; }

    template<typename T>
    void addGenericTimeline(const RHSHARED<T>& timeline) {
        type t = type_get<T>();
        auto it = generic_timelines.find(t);
        if (it != generic_timelines.end()) {
            assert(false);
            return;
        }
        auto ptr = new animGenericTimelineContainerT<T>();
        ptr->ref = timeline;
        generic_timelines.insert(std::make_pair(type_get<T>(), std::unique_ptr<animGenericTimelineContainer>(ptr)));
    }
    template<typename T>
    T* getGenericTimeline() {
        type t = type_get<T>();
        auto it = generic_timelines.find(t);
        if (it == generic_timelines.end()) {
            return 0;
        }
        return it->second.get();
    }
};