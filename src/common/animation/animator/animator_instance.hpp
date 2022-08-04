#pragma once

#include <unordered_map>
#include <memory>
#include "reflection/reflection.hpp"

#include "animation/animator/animator.hpp"
#include "animation/animator/components/animator_component.hpp"


class animAnimatorInstance {
    AnimatorEd* animator = 0;
    sklSkeletonInstance* skl_inst = 0;
    std::unordered_map<type, std::unique_ptr<animAnimatorComponent>> components;

    std::unordered_map<int, float>          parameters;
    std::unordered_map<int, bool>           signals;
    std::unordered_map<int, bool>           feedback_events;
public:
    template<typename T>
    T* addComponent() {
        auto it = components.find(type_get<T>());
        if (it != components.end()) {
            assert(false);
            return 0;
        }
        auto ptr = new T();
        components.insert(std::make_pair(type_get<T>(), std::unique_ptr<animAnimatorComponent>(ptr)));
        return ptr;
    }

    void init(AnimatorEd* animator, sklSkeletonInstance* skl_inst) {
        this->animator = animator;
        this->skl_inst = skl_inst;
    }

    void update(float dt) {/*
        if (!rootUnit) {
            return;
        }

        // Clear feedback events
        for (auto& kv : feedback_events) {
            kv.second = false;
        }

        // Clear sampler influence weights
        for (auto& sg : sync_groups) {
            sg.second->clearInfluence();
        }
        // Calc new influence weights
        rootUnit->updateInfluence(this, 1.0f);
        // Update sampler length scale based on influence weights
        for (auto& sg : sync_groups) {
            sg.second->updateLengths();
        }
        // Sample animations based on scaled length and influence weights (.0f skips sampling but advances cursor)
        for (auto& sg : sync_groups) {
            sg.second->sampleClips(dt);
        }
        // Update animator tree
        rootUnit->update(this, &samples, dt);

        // Clear signals
        for (auto& kv : signals) {
            kv.second = false;
        }*/
    }
};