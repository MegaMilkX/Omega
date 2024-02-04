#pragma once

#include "animation/util/util.hpp"

#include "animation/animator/anim_unit.hpp"


class animFsmState;
struct animFsmTransition {
    animFsmState*   target;
    std::string     expression;
    int             expr_addr = -1;
    float           rate_seconds;
};

class animUnitFsm;
class animFsmState {
    friend animUnitFsm;

    std::string name;
    std::unique_ptr<animUnit> unit;
    std::vector<animFsmTransition> transitions;
    std::string expr_on_exit;
    int expr_on_exit_addr = -1;

public:
    void setUnit(animUnit* unit) {
        this->unit.reset(unit);
    }

    void addTransition(animFsmState* target, const char* expression, float rate_seconds) {
        transitions.push_back(animFsmTransition{ target, expression, -1, rate_seconds });
    }

    void onExit(const char* expression) {
        expr_on_exit = MKSTR(expression << "; return 0;");
    }

    bool isAnimFinished(AnimatorInstance* anim_inst) const {
        return unit->isAnimFinished(anim_inst);
    }

    bool compile(animGraphCompileContext* ctx, AnimatorMaster* animator, Skeleton* skl);

    void prepareInstance(AnimatorInstance* inst) {
        unit->prepareInstance(inst);
    }

    void updateInfluence(AnimatorMaster* master, AnimatorInstance* anim_inst, float infl) {
        unit->updateInfluence(master, anim_inst, infl);
    }
    void update(AnimatorInstance* anim_inst, animSampleBuffer* samples, float dt) {
        unit->update(anim_inst, samples, dt);
    }
};


class animUnitFsm : public animUnit {
    int idx = -1;

    std::unordered_map<std::string, std::unique_ptr<animFsmState>> states;

    // TODO: Remove, this is per instance data
    //float transition_rate = .0f;
    //animSampleBuffer latest_state_samples;
    //animFsmState* current_state = 0;
    //float transition_factor = .0f;
    //bool  is_transitioning = false;
    //

    std::vector<animFsmTransition> global_transitions;

public:
    animUnitFsm() {}

    animFsmState* addState(const char* name) {
        auto ptr = new animFsmState();
        ptr->name = name;
        states.insert(std::make_pair(std::string(name), std::unique_ptr<animFsmState>(ptr)));
        return ptr;
    }
    void addTransition(const char* from, const char* to, const char* expression, float rate_seconds) {
        auto it_a = states.find(from);
        auto it_b = states.find(to);
        if (it_a == states.end() || it_b == states.end()) {
            assert(false);
            return;
        }
        std::string expr = MKSTR("return " << expression << ";");
        it_a->second->addTransition(it_b->second.get(), expr.c_str(), rate_seconds);
    }
    void addTransitionAnySource(const char* to, const char* expression, float rate_seconds) {
        auto it_b = states.find(to);
        if (it_b == states.end()) {
            assert(false);
            return;
        }
        std::string expr = MKSTR("return " << expression << ";");
        global_transitions.push_back(animFsmTransition{ it_b->second.get(), expr, -1, rate_seconds });
    }

    bool compile(animGraphCompileContext* ctx, AnimatorMaster* animator, Skeleton* skl) override;
    
    void prepareInstance(AnimatorInstance* inst) {
        if (!states.empty()) {
            inst->getData()->fsm_data[idx].current_state = states.begin()->second.get();
        }

        inst->getData()->fsm_data[idx].latest_state_samples.init(inst->getSkeletonMaster());

        for (auto& kv : states) {
            kv.second->prepareInstance(inst);
        }
    }

    void updateInfluence(AnimatorMaster* master, AnimatorInstance* anim_inst, float infl) override {
        anim_inst->getData()->fsm_data[idx].current_state->updateInfluence(master, anim_inst, infl);
    }

    void update(AnimatorInstance* anim_inst, animSampleBuffer* samples, float dt) override {
        auto& data = anim_inst->getData()->fsm_data[idx];

        // TODO: Check that transitions are properly triggering and behaving even during another transition
        if (data.is_transitioning) {
            data.current_state->update(anim_inst, samples, dt);
            animBlendSamples(data.latest_state_samples, *samples, *samples, data.transition_factor);
            data.transition_factor += data.transition_rate * dt;
            if (data.transition_factor > 1.0f) {
                data.is_transitioning = false;
            }
        } else {
            data.current_state->update(anim_inst, &data.latest_state_samples, dt);
            samples->copy(data.latest_state_samples);
        }

        // Global transitions first
        for (int i = 0; i < global_transitions.size(); ++i) {
            auto& tr = global_transitions[i];
            if(anim_inst->runExpr(tr.expr_addr)) {
                anim_inst->runExpr(data.current_state->expr_on_exit_addr);
                
                data.current_state = tr.target;
                data.transition_rate = 1.0f / tr.rate_seconds;
                data.transition_factor = .0f;
                data.is_transitioning = true;
                return;
            }
        }
        // Evaluate state transitions after global ones
        for (int i = 0; i < data.current_state->transitions.size(); ++i) {
            auto& tr = data.current_state->transitions[i];
            if(anim_inst->runExpr(tr.expr_addr)) {
                anim_inst->runExpr(data.current_state->expr_on_exit_addr);

                data.current_state = tr.target;
                data.transition_rate = 1.0f / tr.rate_seconds;
                data.transition_factor = .0f;
                data.is_transitioning = true;
                return;
            }
        }
    }
};