#pragma once

#include <unordered_map>
#include <string>
#include "handle/hshared.hpp"
#include "skeleton/skeleton_editable.hpp"
#include "anim_unit.hpp"
#include "animation/animation_sample_buffer.hpp"
#include "animation/animator/anim_unit_single.hpp"
#include "animation/animator/anim_unit_fsm/anim_unit_fsm.hpp"
#include "animation/animator/anim_unit_blend_tree/anim_unit_blend_tree.hpp"

#include "animation/animator/animator_sync_group.hpp"

#include "animation/animator/animator_instance.hpp"

class AnimatorMaster {
    RHSHARED<sklSkeletonMaster> skeleton;
    
    struct SamplerDesc {
        std::string name;
        std::string sync_group;
        RHSHARED<animSequence> sequence;
    };
    std::vector<SamplerDesc> samplers;
    std::unordered_map<std::string, int> sampler_names;

    std::unique_ptr<animUnit> rootUnit;

    std::unordered_map<std::string, int>    param_names;
    std::unordered_map<std::string, int>    signal_names;
    std::unordered_map<std::string, int>    feedback_event_names;
    // TODO: Output values?

    // 
    std::set<HSHARED<animAnimatorInstance>> instances;

public:
    AnimatorMaster() {}

    /// Edit-time
    void setSkeleton(RHSHARED<sklSkeletonMaster> skl) {
        skeleton = skl;
    }
    sklSkeletonMaster* getSkeleton() { return skeleton.get(); }

    AnimatorMaster& addSampler(const char* name, const char* sync_group, const RHSHARED<animSequence>& sequence) {
        auto it = sampler_names.find(name);
        if (it != sampler_names.end()) {
            assert(false);
            return *this;
        }
        sampler_names[name] = samplers.size();
        samplers.push_back(SamplerDesc{ name, sync_group, sequence });
        return *this;
    }
    int getSamplerId(const char* name) {
        auto it = sampler_names.find(name);
        if (it == sampler_names.end()) {
            assert(false);
            return -1;
        }
        return it->second;
    }

    template<typename T>
    T* setRoot() {
        auto ptr = new T();
        rootUnit.reset(ptr);
        return ptr;
    }
    animUnit* getRoot() { return rootUnit.get(); }

    int addSignal(const char* name) {
        return getSignalId(name);
    }
    int getSignalId(const char* name) {
        static int next_id = 1;
        auto it = signal_names.find(name);
        if (it == signal_names.end()) {
            int id = next_id++;
            signal_names.insert(std::make_pair(std::string(name), id));
            return id;
        }
        return it->second;
    }

    int addFeedbackEvent(const char* name) { return getFeedbackEventId(name); }
    int getFeedbackEventId(const char* name) {
        static int next_id = 1;
        auto it = feedback_event_names.find(name);
        if (it == feedback_event_names.end()) {
            int id = next_id++;
            feedback_event_names.insert(std::make_pair(std::string(name), id));
            return id;
        }
        return it->second;
    }

    int addParam(const char* name) {
        return getParamId(name);
    }
    int getParamId(const char* name) {
        static int next_id = 1;
        auto it = param_names.find(name);
        if (it == param_names.end()) {
            int id = next_id++;
            param_names.insert(std::make_pair(std::string(name), id));
            return id;
        }
        return it->second;
    }

    bool compile() {
        assert(skeleton);
        assert(rootUnit);
        if (!skeleton || !rootUnit) {
            LOG_ERR("AnimatorMaster missing skeleton or rootUnit");
            return false;
        }
        // Init animator tree
        rootUnit->compile(this, skeleton.get());
        return true;
    }
    HSHARED<animAnimatorInstance> createInstance();
};