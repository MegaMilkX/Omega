#pragma once

#include <unordered_map>
#include <memory>
#include "reflection/reflection.hpp"

#include "animation/animator/components/animator_component.hpp"

#include "animation/animation_sample_buffer.hpp"
#include "animation/hitbox_sequence/hitbox_seq_sample_buffer.hpp"
#include "animation/audio_sequence/audio_cmd_buffer.hpp"


class AnimatorMaster;
class animAnimatorInstance {
    friend AnimatorMaster;

    AnimatorMaster* animator = 0;

    std::unordered_map<int, float>          parameters;
    std::unordered_map<int, bool>           signals;
    std::unordered_map<int, bool>           feedback_events;

    std::vector<animAnimatorSampler> samplers;
    std::unordered_map<std::string, std::unique_ptr<animAnimatorSyncGroup>> sync_groups;
    std::vector<animAnimatorSyncGroup*> sync_groups_hitbox;
    std::vector<animAnimatorSyncGroup*> sync_groups_audio;

    animSampleBuffer samples;
    hitboxCmdBuffer  hitbox_buffer;
    audioCmdBuffer audio_cmd_buffer;

public:
    Skeleton* getSkeletonMaster();

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

    animAnimatorSampler* getSampler(int id) {
        assert(id >= 0 && id < samplers.size());
        return &samplers[id];
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

    void update(float dt);

    animSampleBuffer* getSampleBuffer() { return &samples; }
    hitboxCmdBuffer* getHitboxCmdBuffer() { return &hitbox_buffer; }
    audioCmdBuffer* getAudioCmdBuffer() { return &audio_cmd_buffer; }
};