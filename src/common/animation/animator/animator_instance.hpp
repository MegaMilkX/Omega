#pragma once

#include <unordered_map>
#include <memory>
#include "reflection/reflection.hpp"

#include "animation/animator/components/animator_component.hpp"

#include "animation/animation_sample_buffer.hpp"
#include "animation/hitbox_sequence/hitbox_seq_sample_buffer.hpp"
#include "animation/audio_sequence/audio_cmd_buffer.hpp"

#include "animation/animvm/animvm.hpp"


class animFsmState;
struct animUnitFsmInstanceData {
    animSampleBuffer latest_state_samples;
    animFsmState* current_state = 0;
    bool is_transitioning = false;
    float transition_rate = .0f;
    float transition_factor = .0f;
};

struct animGraphInstanceData {
    std::vector<animUnitFsmInstanceData> fsm_data;
};

class AnimatorMaster;
class AnimatorInstance {
    friend AnimatorMaster;

    AnimatorMaster* animator = 0;

    // Virtual machine for running transition conditions
    // and triggering host events
    animvm::VM vm;
    // and a copy of the program from AnimatorMaster
    animvm::vm_program vm_program;

    std::unordered_map<int, bool>           feedback_events;
    
    std::vector<animAnimatorSampler> samplers;
    std::unordered_map<std::string, std::unique_ptr<animAnimatorSyncGroup>> sync_groups;
    std::vector<animAnimatorSyncGroup*> sync_groups_hitbox;
    std::vector<animAnimatorSyncGroup*> sync_groups_audio;

    animSampleBuffer samples;
    hitboxCmdBuffer  hitbox_buffer;
    audioCmdBuffer audio_cmd_buffer;

    animGraphInstanceData instance_data;

    void onHostEventCb(int id) {
        LOG("Host event " << id);
        Beep(300, 50);
        const auto& it = feedback_events.find(id);
        if (it == feedback_events.end()) {
            assert(false);
            LOG_ERR("Host event " << id << " does not exist");
            return;
        }
        it->second = true;
    }

public:
    Skeleton* getSkeletonMaster();

    int runExpr(int addr) {
        if (addr < 0) {
            assert(false);
            return 0;
        }
        int ret = vm.run_at(addr);
        vm.clear_stack();
        return ret;
    }

    animAnimatorSampler* getSampler(int id) {
        assert(id >= 0 && id < samplers.size());
        return &samplers[id];
    }

    void triggerSignal(int id) {
        vm_program.set_variable_bool(id, true);
    }
    bool isSignalTriggered(int id) {
        return vm_program.get_variable_bool(id);
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
        vm_program.set_variable_float(/*addr*/id, val);
    }
    float getParamValue(int id) const {
        return vm_program.get_variable_float(id);
    }

    void update(float dt);

    animGraphInstanceData* getData() { return &instance_data; }

    animSampleBuffer* getSampleBuffer() { return &samples; }
    hitboxCmdBuffer* getHitboxCmdBuffer() { return &hitbox_buffer; }
    audioCmdBuffer* getAudioCmdBuffer() { return &audio_cmd_buffer; }
};