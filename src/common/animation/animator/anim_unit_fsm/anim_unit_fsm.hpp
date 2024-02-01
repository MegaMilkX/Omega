#pragma once

#include "animation/animator/expr/expr.hpp"
#include "animation/util/util.hpp"

#include "animation/animator/anim_unit.hpp"

class animUnitFsm;
class animFsmState;
struct animFsmTransition {
    animFsmState*   target;
    std::string     expression;
    int             expr_addr = -1;
    float           rate_seconds;
};
class animFsmState {
    friend animUnitFsm;

    std::string name;
    std::unique_ptr<animUnit> unit;
    std::vector<animFsmTransition> transitions;
    uint32_t current_cycle = 0;
    std::string expr_on_exit;
    int expr_on_exit_addr = -1;
public:
    template<typename T>
    T* setUnit() {
        auto ptr = new T();
        unit.reset(ptr);
        return ptr;
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

    bool compile(AnimatorMaster* animator, Skeleton* skl);

    void updateInfluence(AnimatorInstance* anim_inst, float infl) {
        unit->updateInfluence(anim_inst, infl);
    }
    void update(AnimatorInstance* anim_inst, animSampleBuffer* samples, float dt) {
        unit->update(anim_inst, samples, dt);
    }
};
class animUnitFsm : public animUnit {
    std::unordered_map<std::string, std::unique_ptr<animFsmState>> states;
    animSampleBuffer latest_state_samples;
    animFsmState* current_state = 0;
    float transition_rate = .0f;
    float transition_factor = .0f;
    bool  is_transitioning = false;

    std::vector<animFsmTransition> global_transitions;

public:
    animUnitFsm() {}

    animFsmState* addState(const char* name) {
        auto ptr = new animFsmState();
        ptr->name = name;
        if (states.empty()) {
            current_state = ptr;
        }
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

    bool compile(AnimatorMaster* animator, Skeleton* skl) override;

    void updateInfluence(AnimatorInstance* anim_inst, float infl) override {
        current_state->updateInfluence(anim_inst, infl);
    }

    void update(AnimatorInstance* anim_inst, animSampleBuffer* samples, float dt) override {
        // TODO: Check that transitions are properly triggering and behaving even during another transition
        if (is_transitioning) {
            current_state->update(anim_inst, samples, dt);
            animBlendSamples(latest_state_samples, *samples, *samples, transition_factor);
            transition_factor += transition_rate * dt;
            if (transition_factor > 1.0f) {
                is_transitioning = false;
            }
        } else {
            current_state->update(anim_inst, &latest_state_samples, dt);
            samples->copy(latest_state_samples);
        }

        // Global transitions first
        for (int i = 0; i < global_transitions.size(); ++i) {
            auto& tr = global_transitions[i];
            if(anim_inst->runExpr(tr.expr_addr)) {
                anim_inst->runExpr(current_state->expr_on_exit_addr);
                
                current_state = tr.target;
                transition_rate = 1.0f / tr.rate_seconds;
                transition_factor = .0f;
                is_transitioning = true;
                return;
            }
        }
        // Evaluate state transitions after global ones
        for (int i = 0; i < current_state->transitions.size(); ++i) {
            auto& tr = current_state->transitions[i];
            if(anim_inst->runExpr(tr.expr_addr)) {
                anim_inst->runExpr(current_state->expr_on_exit_addr);

                current_state = tr.target;
                transition_rate = 1.0f / tr.rate_seconds;
                transition_factor = .0f;
                is_transitioning = true;
                return;
            }
        }
    }
};