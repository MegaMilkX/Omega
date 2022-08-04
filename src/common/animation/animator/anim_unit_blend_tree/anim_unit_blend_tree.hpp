#pragma once



class AnimatorEd;
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
    virtual bool compile(sklSkeletonEditable* skl, std::set<animBtNode*>& exec_set, int order) = 0;
    virtual void propagateInfluence(AnimatorEd* animator, float influence) = 0;
    virtual void update(AnimatorEd* animator, float dt) = 0;
    virtual animSampleBuffer* getOutputSamples() = 0;
};
class animUnitBlendTree;
class animBtNodeClip : public animBtNode {
    /*friend animUnitBlendTree;
    RHSHARED<Animation> anim;
    animSampler         sampler;
    animSampleBuffer    samples;
    float cursor_prev = .0f;
    float cursor = .0f;
    float length_scaled = .0f;*/
    animAnimatorSampler* sampler_;

public:
    //void setAnimation(const RHSHARED<Animation>& anim) { this->anim = anim; }
    void setSampler(animAnimatorSampler* smp) { sampler_ = smp; }

    bool compile(sklSkeletonEditable* skl, std::set<animBtNode*>& exec_set, int order) override {
        /*
        if (!anim) {
            LOG_ERR("animBtNodeClip::compile(): animation is null");
            assert(false);
            return false;
        }
        updatePriority(order);
        sampler = animSampler(skl, anim.get());
        samples.init(skl);
        exec_set.insert(this);*/
        if (!sampler_) {
            assert(false);
            return false;
        }
        return true;
    }
    void propagateInfluence(AnimatorEd* animator, float influence) override {
        sampler_->propagateInfluence(influence);
    }
    /*
    void propagateInfluence(AnimatorEd* animator, float influence) override {
        total_influence += influence;
    }
    void sampleAndAdvance(float dt, bool sample_new) {
        if (cursor > anim->length) {
            cursor -= anim->length;
        }
        if (sample_new) {
            if (anim->hasRootMotion()) {
                samples.has_root_motion = true;
                sampler.sampleWithRootMotion(samples.data(), samples.count(), cursor_prev, cursor, &samples.getRootMotionSample());
            } else {
                samples.has_root_motion = false;
                sampler.sample(samples.data(), samples.count(), cursor);
            }
        }
        cursor_prev = cursor;
        cursor += (anim->length / length_scaled) * dt * anim->fps;
    }*/
    void update(AnimatorEd* animator, float dt) override {/*
        // Do nothing here?
        if (cursor > anim->length) {
            cursor -= anim->length;
        }
        sampler.sample(samples.data(), samples.count(), cursor);
        cursor += dt * anim->fps;*/
    }
    animSampleBuffer* getOutputSamples() override {
        //return &samples;
        return &sampler_->samples;
    }
};
class animBtNodeFrame : public animBtNode {
    animSampleBuffer samples;
public:
    RHSHARED<Animation>  anim;
    int                  keyframe;
    bool compile(sklSkeletonEditable* skl, std::set<animBtNode*>& exec_set, int order) override {
        // TODO
    }
    void propagateInfluence(AnimatorEd* animator, float influence) override {
        // TODO
    }
    void update(AnimatorEd* animator, float dt) override {}
    animSampleBuffer* getOutputSamples() override {
        // TODO
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

    bool compile(sklSkeletonEditable* skl, std::set<animBtNode*>& exec_set, int order) override {
        if (in_a == nullptr || in_b == nullptr) {
            LOG_ERR("animBtNodeBlend2::compile(): input is incomplete (a or b is null)");
            assert(false);
            return false;
        }
        updatePriority(order);
        samples.init(skl);
        in_a->compile(skl, exec_set, order + 1);
        in_b->compile(skl, exec_set, order + 1);
        exec_set.insert(this);
        return true;
    }
    void propagateInfluence(AnimatorEd* animator, float influence) override {
        total_influence += influence;
        weight = weight_expression.evaluate(animator).to_float();
        in_a->propagateInfluence(animator, total_influence * (1.0f - weight));
        in_b->propagateInfluence(animator, total_influence * weight);
    }
    void update(AnimatorEd* animator, float dt) override {        
        animBlendSamples(*in_a->getOutputSamples(), *in_b->getOutputSamples(), samples, weight);
    }
    animSampleBuffer* getOutputSamples() override {
        return &samples;
    }
};
class animUnitBlendTree : public animUnit {
    std::set<animBtNode*> nodes;
    //std::vector<animBtNodeClip*> clip_nodes;
    animSampleBuffer samples;
    animBtNode* out_node = 0;
    std::vector<animBtNode*> exec_chain;
    /*
    void propagateInfluence(AnimatorEd* animator) {
        out_node->propagateInfluence(animator, 1.0f);
    }*/

public:
    template<typename T>
    T* addNode() {
        auto ptr = new T();
        nodes.insert(ptr);
        return ptr;
    }/*
    template<>
    animBtNodeClip* addNode<animBtNodeClip>() {
        auto ptr = new animBtNodeClip();
        nodes.insert(ptr);
        clip_nodes.push_back(ptr);
        return ptr;
    }*/

    void setOutputNode(animBtNode* node) { out_node = node; }

    bool compile(sklSkeletonEditable* skl) override {
        if (out_node == nullptr) {
            LOG_ERR("animUnitBlendTree::compile(): output node is null");
            assert(false);
            return false;
        }
        samples.init(skl);
        std::set<animBtNode*> exec_set;
        out_node->compile(skl, exec_set, 0);
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
    void updateInfluence(AnimatorEd* animator, float infl) override {
        out_node->propagateInfluence(animator, infl);
    }
    void update(AnimatorEd* animator, animSampleBuffer* samples, float dt) override {
        // Clear influence
        /*
        for (auto it : nodes) {
            it->total_influence = .0f;            
        }
        //propagateInfluence(animator);
        // Sort clips by lowest influence first        
        std::sort(clip_nodes.begin(), clip_nodes.end(), [](const animBtNodeClip* a, const animBtNodeClip* b)->bool {
            return a->total_influence < b->total_influence;
        });
        // Set clip advance speeds
        if (!clip_nodes.empty()) {
            clip_nodes[0]->length_scaled = clip_nodes[0]->anim->length;
            for (int i = 1; i < clip_nodes.size(); ++i) {
                auto clip_a = clip_nodes[i - 1];
                auto clip_b = clip_nodes[i];                
                auto infl_a = clip_a->total_influence;
                auto infl_b = clip_b->total_influence;
                auto n_infl_a = infl_a / (infl_a + infl_b);
                auto n_infl_b = infl_b / (infl_a + infl_b);
                auto spd_weight = n_infl_b;
                clip_a->length_scaled = gfxm::lerp(clip_a->anim->length, clip_b->anim->length, 1.0 - spd_weight);
                clip_b->length_scaled = gfxm::lerp(clip_a->anim->length, clip_b->anim->length, spd_weight);
            }
        }
        // Sample clips
        for (int i = 0; i < clip_nodes.size(); ++i) {
            auto clip = clip_nodes[i];
            clip->sampleAndAdvance(dt, clip->total_influence > FLT_EPSILON);
        }*/
        // Exec chain
        for (int i = 0; i < exec_chain.size(); ++i) {
            auto node = exec_chain[i];
            node->update(animator, dt);
        }
        // Get result
        samples->copy(*out_node->getOutputSamples());
    }
};