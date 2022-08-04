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

#include "animation/animator/components/animator_component.hpp"

class AnimatorEd {
    RHSHARED<sklSkeletonEditable> skeleton;
    
    std::unordered_map<std::string, int> sync_group_names;
    std::unordered_map<int, std::unique_ptr<animAnimatorSyncGroup>> sync_groups;
    
    std::unique_ptr<animUnit> rootUnit;
    animSampleBuffer samples;

    std::unordered_map<int, float>          parameters;
    std::unordered_map<std::string, int>    param_names;
    std::unordered_map<int, bool>           signals;
    std::unordered_map<std::string, int>    signal_names;
    std::unordered_map<std::string, int>    feedback_event_names;
    std::unordered_map<int, bool>           feedback_events;
    // TODO: Output values?


public:
    AnimatorEd() {}

    /// Edit-time
    void setSkeleton(RHSHARED<sklSkeletonEditable> skl) {
        skeleton = skl;
        samples.init(skeleton.get());
    }

    animAnimatorSyncGroup* createSyncGroup(const char* name) {
        auto it_name = sync_group_names.find(name);
        if (it_name != sync_group_names.end()) {
            assert(false);
            return 0;
        }
        static int next_id = 1;
        animAnimatorSyncGroup* grp = new animAnimatorSyncGroup();
        sync_group_names.insert(std::make_pair(std::string(name), next_id));
        sync_groups.insert(std::make_pair(next_id, std::unique_ptr<animAnimatorSyncGroup>(grp)));
        next_id++;
        return grp;
    }

    template<typename T>
    T* setRoot() {
        auto ptr = new T();
        rootUnit.reset(ptr);
        return ptr;
    }

    int addSignal(const char* name) {
        return getSignalId(name);
    }
    int getSignalId(const char* name) {
        static int next_id = 1;
        auto it = signal_names.find(name);
        if (it == signal_names.end()) {
            int id = next_id++;
            signal_names.insert(std::make_pair(std::string(name), id));
            signals.insert(std::make_pair(id, false));
            return id;
        }
        return it->second;
    }
    void triggerSignal(int id) {
        auto it = signals.find(id);
        if (it == signals.end()) {
            LOG_ERR("triggerSignal: No such signal: " << id);
            assert(false);
            return;
        }
        it->second = true;
    }
    bool isSignalTriggered(int id) {
        auto it = signals.find(id);
        if (it == signals.end()) {
            LOG_ERR("isSignalTriggered: no such signal: " << id);
            assert(false);
            return false;
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
            feedback_events.insert(std::make_pair(id, false));
            return id;
        }
        return it->second;
    }
    void triggerFeedbackEvent(int id) {
        auto it = feedback_events.find(id);
        if (it == feedback_events.end()) {
            LOG_ERR("triggerFeedbackEvent: No such event: " << id);
            assert(false);
            return;
        }
        it->second = true;
    }
    bool isFeedbackEventTriggered(int id) {
        auto it = feedback_events.find(id);
        if (it == feedback_events.end()) {
            LOG_ERR("isFeedbackEventTriggered: no such event: " << id);
            assert(false);
            return false;
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
            parameters.insert(std::make_pair(id, .0f));
            return id;
        }
        return it->second;
    }
    void setParamValue(int id, float val) {
        auto it = parameters.find(id);
        if (it == parameters.end()) {
            LOG_ERR("setParamValue: No such parameter: " << id);
            assert(false);
            return;
        }
        it->second = val;
    }
    float getParamValue(int id) const {
        auto it = parameters.find(id);
        if (it == parameters.end()) {
            LOG_ERR("getParamValue: No such parameter: " << id);
            assert(it != parameters.end());
            return .0f;
        }
        return it->second;
    }
    expr_ getParamExpr(const char* name) {
        expr_ ret;
        ret.type = EXPR_PARAM;
        static int next_id = 1;
        auto it = param_names.find(name);
        if (it == param_names.end()) {
            int id = next_id++;
            param_names.insert(std::make_pair(std::string(name), id));
            parameters.insert(std::make_pair(id, .0f));
            ret.param_index = id;
        }
        ret.param_index = it->second;
        return ret;
    }

    bool compile() {
        assert(skeleton);
        assert(rootUnit);
        if (!skeleton || !rootUnit) {
            return false;
        }
        // Init sync groups
        for (auto& sg : sync_groups) {
            if (!sg.second->compile(skeleton.get())) {
                assert(false);
                return false;
            }
        }
        // Init animator tree
        rootUnit->compile(skeleton.get());
        return true;
    }

    /// Runtime
    void update(float dt) {
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
        }
    }

    animSampleBuffer* getSampleBuffer() {
        return &samples;
    }
};