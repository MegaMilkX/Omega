#pragma once

#include <assert.h>
#include <set>
#include "log/log.hpp"

#include "animation/animator/expr/expr.hpp"
#include "animation/animation_sample_buffer.hpp"
#include "animation/animator/animator_instance.hpp"
#include "animation/util/util.hpp"
#include "animation/animator/anim_unit.hpp"

class AnimatorMaster;
class animAnimatorInstance;
class animBtNode {
protected:
    void updatePriority(int prio) {
        if (exec_priority < prio) {
            exec_priority = prio;
        }
    }
public:
    int exec_priority = 0;
    float total_influence = .0f;

    virtual ~animBtNode() {}
    virtual bool compile(AnimatorMaster* animator, std::set<animBtNode*>& exec_set, int order) = 0;
    virtual void propagateInfluence(animAnimatorInstance* anim_inst, float influence) = 0;
    virtual void update(animAnimatorInstance* anim_inst, float dt) = 0;
    virtual animSampleBuffer* getOutputSamples(animAnimatorInstance* anim_inst) = 0;
};
class animUnitBlendTree;
class animBtNodeClip : public animBtNode {
    std::string sampler_name;
    int sampler_id = -1;
public:
    //void setAnimation(const RHSHARED<Animation>& anim) { this->anim = anim; }
    void setSampler(const char* name) { sampler_name = name; }

    bool compile(AnimatorMaster* animator, std::set<animBtNode*>& exec_set, int order) override;
    void propagateInfluence(animAnimatorInstance* anim_inst, float influence) override {
        anim_inst->getSampler(sampler_id)->propagateInfluence(influence);
    }
    void update(animAnimatorInstance* anim_inst, float dt) override {
        // TODO: Do nothing here?
    }
    animSampleBuffer* getOutputSamples(animAnimatorInstance* anim_inst) override {
        return &anim_inst->getSampler(sampler_id)->samples;
    }
};
class animBtNodeFrame : public animBtNode {
    animSampleBuffer samples;
public:
    RHSHARED<Animation>  anim;
    int                  keyframe;
    bool compile(AnimatorMaster* animator, std::set<animBtNode*>& exec_set, int order) override;
    void propagateInfluence(animAnimatorInstance* anim_inst, float influence) override {
        // TODO
    }
    void update(animAnimatorInstance* anim_inst, float dt) override {}
    animSampleBuffer* getOutputSamples(animAnimatorInstance* anim_inst) override {
        // TODO
        return 0;
    }
};
class animBtNodeBlend2 : public animBtNode {
    animBtNode* in_a;
    animBtNode* in_b;
    expr_ weight_expression;
    float weight = .0f;
    animSampleBuffer samples;
public:
    void setInputs(animBtNode* a, animBtNode* b) {
        in_a = a;
        in_b = b;
    }
    void setWeightExpression(const expr_& e) {
        weight_expression = e;
    }

    bool compile(AnimatorMaster* animator, std::set<animBtNode*>& exec_set, int order) override;
    void propagateInfluence(animAnimatorInstance* anim_inst, float influence) override {
        total_influence += influence;
        weight = weight_expression.evaluate(anim_inst).to_float();
        in_a->propagateInfluence(anim_inst, total_influence * (1.0f - weight));
        in_b->propagateInfluence(anim_inst, total_influence * weight);
    }
    void update(animAnimatorInstance* anim_inst, float dt) override {
        animBlendSamples(*in_a->getOutputSamples(anim_inst), *in_b->getOutputSamples(anim_inst), samples, weight);
    }
    animSampleBuffer* getOutputSamples(animAnimatorInstance* anim_inst) override {
        return &samples;
    }
};
class animUnitBlendTree : public animUnit {
    std::set<animBtNode*> nodes;
    animSampleBuffer samples;
    animBtNode* out_node = 0;
    std::vector<animBtNode*> exec_chain;

public:
    template<typename T>
    T* addNode() {
        auto ptr = new T();
        nodes.insert(ptr);
        return ptr;
    }

    void setOutputNode(animBtNode* node) { out_node = node; }

    bool compile(AnimatorMaster* animator, sklSkeletonMaster* skl) override {
        if (out_node == nullptr) {
            LOG_ERR("animUnitBlendTree::compile(): output node is null");
            assert(false);
            return false;
        }
        samples.init(skl);
        std::set<animBtNode*> exec_set;
        out_node->compile(animator, exec_set, 0);
        exec_chain.resize(exec_set.size());
        int i = 0;
        for (auto& it : exec_set) {
            exec_chain[i] = it;
            ++i;
        }
        std::sort(exec_chain.begin(), exec_chain.end(), [](const animBtNode* a, const animBtNode* b)->bool {
            return a->exec_priority > b->exec_priority;
        });
        for (int i = 0; i < exec_chain.size(); ++i) {
            LOG_DBG(exec_chain[i]->exec_priority);
        }
        return true;
    }
    void updateInfluence(animAnimatorInstance* anim_inst, float infl) override {
        out_node->propagateInfluence(anim_inst, infl);
    }
    void update(animAnimatorInstance* anim_inst, animSampleBuffer* samples, float dt) override {
        // Exec chain
        for (int i = 0; i < exec_chain.size(); ++i) {
            auto node = exec_chain[i];
            node->update(anim_inst, dt);
        }
        // Get result
        samples->copy(*out_node->getOutputSamples(anim_inst));
    }
};