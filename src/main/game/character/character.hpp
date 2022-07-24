#pragma once

#include "game/actor/actor.hpp"

#include "assimp_load_scene.hpp"
#include "animation/animation.hpp"
#include "common/collision/collision_world.hpp"
#include "gpu/gpu.hpp"
#include "gpu/render/uniform.hpp"
#include "game/animator/animation_sampler.hpp"
#include "game/animator/animator.hpp"
#include "game/world/world.hpp"

#include "skeletal_model/skeletal_model.hpp"
#include "skeletal_model/skeletal_model_instance.hpp"


class animSampleBuffer {
    std::vector<AnimSample> samples;
public:
    void init(sklSkeletonEditable* skl) {
        samples.resize(skl->boneCount());
        for (int i = 0; i < skl->boneCount(); ++i) {
            auto bone = skl->getBone(i);
            samples[i].t = bone->getLclTranslation();
            samples[i].r = bone->getLclRotation();
            samples[i].s = bone->getLclScale();
        }
    }

    void copy(const animSampleBuffer& other) {
        if (other.count() != count()) {
            assert(false);
            return;
        }
        memcpy(samples.data(), other.data(), samples.size() * sizeof(samples[0]));
    }

    void applySamples(sklSkeletonInstance* skl_inst) {
        for (int i = 1; i < samples.size(); ++i) {
            auto& s = samples[i];
            gfxm::mat4 m = gfxm::translate(gfxm::mat4(1.0f), s.t)
                * gfxm::to_mat4(s.r)
                * gfxm::scale(gfxm::mat4(1.0f), s.s);
            skl_inst->getLocalTransformsPtr()[i] = m;
        }
    }

    size_t count() const { return samples.size(); }
    AnimSample& operator[](int index) { return samples[index]; }
    const AnimSample& operator[](int index) const { return samples[index]; }
    AnimSample* data() { return &samples[0]; }
    const AnimSample* data() const { return &samples[0]; }
};
inline void animBlendSamples(animSampleBuffer& from, animSampleBuffer& to, animSampleBuffer& result, float factor) {
    if (from.count() != to.count() && to.count() != result.count()) {
        assert(false);
        return;
    }
    for (int i = 0; i < from.count(); ++i) {
        auto& s0 = from[i];
        auto& s1 = to[i];
        result[i].t = gfxm::lerp(s0.t, s1.t, factor);
        result[i].s = gfxm::lerp(s0.s, s1.s, factor);
        result[i].r = gfxm::slerp(s0.r, s1.r, factor);
    }
}


class AnimatorEd;
class animUnit {
protected:
    uint32_t current_loop_cycle = 0;
public:
    virtual ~animUnit() {}

    virtual bool isAnimFinished() const { return false; };

    virtual void update(AnimatorEd* animator, animSampleBuffer* samples, float dt) = 0;
    virtual bool compile(sklSkeletonEditable* skl) = 0;
};
class animUnitSingle : public animUnit {
    RHSHARED<Animation> animation;
    animSampler sampler;
    float cursor = .0f;
public:
    animUnitSingle() {}

    void setAnimation(const RHSHARED<Animation>& anim) {
        animation = anim;
    }

    bool isAnimFinished() const override { return cursor >= animation->length; }

    void update(AnimatorEd* animator, animSampleBuffer* samples, float dt) override {
        if (cursor > animation->length) {
            cursor -= animation->length;
        }
        sampler.sample(samples->data(), samples->count(), cursor);
        cursor += dt * animation->fps;
    }
    bool compile(sklSkeletonEditable* skl) override {
        if (!animation) {
            assert(false);
            return false;
        }
        sampler = animSampler(skl, animation.get());
        return true;
    }
};

enum CONDITION_TYPE {
    COND_EQ,
    COND_NOTEQ,
    COND_LESS,
    COND_MORE,
    COND_LESSEQ,
    COND_MOREEQ,

    COND_ANIM_END
};
struct condition_ {
    CONDITION_TYPE type;
    std::shared_ptr<condition_> left;
    std::shared_ptr<condition_> right;
    union {
        float   float_value;
        int     param_id;
        int     event_id;
    };
};

struct cond_anim_end {
    operator condition_() {
        return condition_{ COND_ANIM_END, 0, 0 };
    }
};

class animUnitFsm;
class animFsmState;
struct animFsmTransition {
    animFsmState*   target;
    condition_      cond;
    float           rate_seconds;
};
class animFsmState {
    friend animUnitFsm;
    std::unique_ptr<animUnit> unit;
    std::vector<animFsmTransition> transitions;
    uint32_t current_cycle = 0;
public:
    template<typename T>
    T* setUnit() {
        auto ptr = new T();
        unit.reset(ptr);
        return ptr;
    }

    void addTransition(animFsmState* target, condition_ cond, float rate_seconds) {
        transitions.push_back(animFsmTransition{ target, cond, rate_seconds });
    }

    bool isAnimFinished() const {
        return unit->isAnimFinished();
    }

    void update(AnimatorEd* animator, animSampleBuffer* samples, float dt) {
        unit->update(animator, samples, dt);
    }
    bool compile(sklSkeletonEditable* skl) {
        if (!unit) {
            return false;
        }
        if (!unit->compile(skl)) {
            return false;
        }
        return true;
    }
};
class animUnitFsm : public animUnit {
    std::unordered_map<std::string, std::unique_ptr<animFsmState>> states;
    animSampleBuffer latest_state_samples;
    animFsmState* current_state = 0;
    float transition_rate = .0f;
    float transition_factor = .0f;
    bool  is_transitioning = false;

    bool evalCondition(animFsmState* state, condition_* cond) {
        bool ret = false;
        switch (cond->type) {
        case COND_EQ:
            break;
        case COND_NOTEQ:
            break;
        case COND_LESS:
            break;
        case COND_MORE:
            break;
        case COND_LESSEQ:
            break;
        case COND_MOREEQ:
            break;
        case COND_ANIM_END:
            ret = state->isAnimFinished();
            break;
        }
        return ret;
    }
public:
    animUnitFsm() {}

    animFsmState* addState(const char* name) {
        auto ptr = new animFsmState();
        if (states.empty()) {
            current_state = ptr;
        }
        states.insert(std::make_pair(std::string(name), std::unique_ptr<animFsmState>(ptr)));
        return ptr;
    }
    void addTransition(const char* from, const char* to, condition_ cond, float rate_seconds) {
        auto it_a = states.find(from);
        auto it_b = states.find(to);
        if (it_a == states.end() || it_b == states.end()) {
            assert(false);
            return;
        }
        it_a->second->addTransition(it_b->second.get(), cond, rate_seconds);
    }

    void update(AnimatorEd* animator, animSampleBuffer* samples, float dt) override {
        if (is_transitioning) {
            current_state->update(animator, samples, dt);
            animBlendSamples(latest_state_samples, *samples, *samples, transition_factor);
            transition_factor += transition_rate * dt;
            if (transition_factor > 1.0f) {
                is_transitioning = false;
            }
        } else {
            current_state->update(animator, &latest_state_samples, dt);
            samples->copy(latest_state_samples);

            for (int i = 0; i < current_state->transitions.size(); ++i) {
                auto& tr = current_state->transitions[i];
                if (evalCondition(current_state, &tr.cond)) {
                    current_state = tr.target;
                    transition_rate = 1.0f / tr.rate_seconds;
                    transition_factor = .0f;
                    is_transitioning = true;
                    break;
                }
            }
        }
    }

    bool compile(sklSkeletonEditable* skl) override {
        if (states.empty() || current_state == 0) {
            assert(false);
            return false;
        }
        for (auto& it : states) {
            if (!it.second->compile(skl)) {
                return false;
            }
        }
        latest_state_samples.init(skl);
        return true;
    }
};

enum VALUE_TYPE {
    VALUE_FLOAT,
    VALUE_FLOAT2,
    VALUE_FLOAT3,
    VALUE_INT,
    VALUE_INT2,
    VALUE_INT3,
    VALUE_BOOL
};
struct value_ {
    VALUE_TYPE type;
    union {
        float       float_;
        gfxm::vec2  float2;
        gfxm::vec3  float3;
        int         int_;
        gfxm::ivec2 int2;
        gfxm::ivec3 int3;
        bool        bool_;
    };
    value_() {}
    value_(float v) : type(VALUE_FLOAT), float_(v) {}
    value_(gfxm::vec2 v) : type(VALUE_FLOAT2), float2(v) {}
    value_(gfxm::vec3 v) : type(VALUE_FLOAT3), float3(v) {}
    value_(int v) : type(VALUE_INT), int_(v) {}
    value_(gfxm::ivec2 v) : type(VALUE_INT2), int2(v) {}
    value_(gfxm::ivec3 v) : type(VALUE_INT3), int3(v) {}
    value_(bool v) : type(VALUE_BOOL), bool_(v) {}

    float to_float() const {
        if (type != VALUE_FLOAT) {
            assert(false);
            return .0f;
        }
        return float_;
    }

    bool convert(VALUE_TYPE type, value_& out) const {
        switch (this->type) {
        case VALUE_FLOAT:
            switch (type) {
            case VALUE_FLOAT: out = *this; return true;
            case VALUE_INT: out = value_(int(float_)); return true;
            case VALUE_BOOL: out = value_(float_ != .0f); return true;
            }
            break;
        case VALUE_FLOAT2:
            switch (type) {
            case VALUE_FLOAT2: out = *this; return true;
            case VALUE_INT2: out = value_(gfxm::ivec2(float2.x, float2.y)); return true;
            }
            break;
        case VALUE_FLOAT3:
            switch (type) {
            case VALUE_FLOAT3: out = *this; return true;
            case VALUE_INT3: out = value_(gfxm::ivec3(float3.x, float3.y, float3.z)); return true;
            }
            break;
        case VALUE_INT:
            switch (type) {
            case VALUE_INT: out = value_(*this); return true;
            case VALUE_FLOAT: out = value_(float(int_)); return true;
            case VALUE_BOOL: out = value_(int_ != 0); return true;
            }
            break;
        case VALUE_INT2:
            switch (type) {
            case VALUE_INT2: out = *this; return true;
            case VALUE_FLOAT2: out = value_(gfxm::vec2(int2.x, int2.y)); return true;
            }
            break;
        case VALUE_INT3:
            switch (type) {
            case VALUE_INT3: out = *this; return true;
            case VALUE_FLOAT3: out = value_(gfxm::vec3(int3.x, int3.y, int3.z)); return true;
            }
            break;
        case VALUE_BOOL:
            switch (type) {
            case VALUE_BOOL: out = *this; return true;
            case VALUE_INT: out = value_(bool_ ? 1 : 0); return true;
            case VALUE_FLOAT: out = value_(bool_ ? 1.0f : .0f); return true;
            }
            break;
        default:
            assert(false);
        }
        return false;
    }

    void dbgLog() const {
        switch (type) {
        case VALUE_FLOAT:
            LOG_DBG("float: " << float_);
            break;
        case VALUE_FLOAT2:
            LOG_DBG("float2: [" << float2.x << ", " << float2.y << "]");
            break;
        case VALUE_FLOAT3:
            LOG_DBG("float3: [" << float3.x << ", " << float3.y << ", " << float3.z << "]");
            break;
        case VALUE_INT:
            LOG_DBG("int: " << int_);
            break;
        case VALUE_INT2:
            LOG_DBG("int2: [" << int2.x << ", " << int2.y << "]");
            break;
        case VALUE_INT3:
            LOG_DBG("int3: [" << int3.x << ", " << int3.y << ", " << int3.z << "]");
            break;
        case VALUE_BOOL:
            LOG_DBG("bool: " << (bool_ ? "true" : "false"));
            break;
        default:
            LOG_DBG("undefined: undefined");
        }
    }
};

class AnimatorEd;
enum EXPR_TYPE {
    EXPR_EQ,
    EXPR_NOTEQ,
    EXPR_LESS,
    EXPR_MORE,
    EXPR_LESSEQ,
    EXPR_MOREEQ,

    EXPR_ADD,
    EXPR_SUB,
    EXPR_MUL,
    EXPR_DIV,
    EXPR_MOD,
    EXPR_ABS,

    EXPR_LIT_FLOAT,
    EXPR_LIT_INT,
    EXPR_LIT_BOOL,

    EXPR_PARAM,
    EXPR_EVENT
};
struct expr_ {
    EXPR_TYPE type;
    std::shared_ptr<expr_> left;
    std::shared_ptr<expr_> right;
    union {
        int     param_index;
        float   float_value;
        int     int_value;
        bool    bool_value;
    };

    expr_() {}
    expr_(EXPR_TYPE type, expr_* left, expr_* right) : type(type), left(left), right(right) {}
    expr_(float val) : expr_(EXPR_LIT_FLOAT, 0, 0) { float_value = val; }
    expr_(int val) : expr_(EXPR_LIT_INT, 0, 0) { int_value = val; }
    expr_(bool val) : expr_(EXPR_LIT_BOOL, 0, 0) { bool_value = val; }

    value_ evaluate(AnimatorEd* animator) const;

    std::string toString(AnimatorEd* animator) const;
    void dbgLog(AnimatorEd* animator) const {
        LOG_DBG(toString(animator));
    }
};
inline expr_ operator+  (const expr_& a, const float& b) { return expr_{ EXPR_ADD, new expr_(a), new expr_(b) }; }
inline expr_ operator-  (const expr_& a, const float& b) { return expr_{ EXPR_SUB, new expr_(a), new expr_(b) }; }
inline expr_ operator*  (const expr_& a, const float& b) { return expr_{ EXPR_MUL, new expr_(a), new expr_(b) }; }
inline expr_ operator/  (const expr_& a, const float& b) { return expr_{ EXPR_DIV, new expr_(a), new expr_(b) }; }
inline expr_ operator%  (const expr_& a, const float& b) { return expr_{ EXPR_MOD, new expr_(a), new expr_(b) }; }

inline expr_ operator+  (const float& a, const expr_& b) { return expr_{ EXPR_ADD, new expr_(a), new expr_(b) }; }
inline expr_ operator-  (const float& a, const expr_& b) { return expr_{ EXPR_SUB, new expr_(a), new expr_(b) }; }
inline expr_ operator*  (const float& a, const expr_& b) { return expr_{ EXPR_MUL, new expr_(a), new expr_(b) }; }
inline expr_ operator/  (const float& a, const expr_& b) { return expr_{ EXPR_DIV, new expr_(a), new expr_(b) }; }
inline expr_ operator%  (const float& a, const expr_& b) { return expr_{ EXPR_MOD, new expr_(a), new expr_(b) }; }

inline expr_ operator+  (const expr_& a, const expr_& b) { return expr_{ EXPR_ADD, new expr_(a), new expr_(b) }; }
inline expr_ operator-  (const expr_& a, const expr_& b) { return expr_{ EXPR_SUB, new expr_(a), new expr_(b) }; }
inline expr_ operator*  (const expr_& a, const expr_& b) { return expr_{ EXPR_MUL, new expr_(a), new expr_(b) }; }
inline expr_ operator/  (const expr_& a, const expr_& b) { return expr_{ EXPR_DIV, new expr_(a), new expr_(b) }; }
inline expr_ operator%  (const expr_& a, const expr_& b) { return expr_{ EXPR_MOD, new expr_(a), new expr_(b) }; }

inline expr_ operator== (const expr_& a, const expr_& b) { return expr_{ EXPR_EQ, new expr_(a), new expr_(b) }; }
inline expr_ operator!= (const expr_& a, const expr_& b) { return expr_{ EXPR_NOTEQ, new expr_(a), new expr_(b) }; }
inline expr_ operator<  (const expr_& a, const expr_& b) { return expr_{ EXPR_LESS, new expr_(a), new expr_(b) }; }
inline expr_ operator>  (const expr_& a, const expr_& b) { return expr_{ EXPR_MORE, new expr_(a), new expr_(b) }; }
inline expr_ operator<= (const expr_& a, const expr_& b) { return expr_{ EXPR_LESSEQ, new expr_(a), new expr_(b) }; }
inline expr_ operator>= (const expr_& a, const expr_& b) { return expr_{ EXPR_MOREEQ, new expr_(a), new expr_(b) }; }

struct param_ {
    int index;
    param_(AnimatorEd* animator, const std::string& name);
    operator expr_() const {
        expr_ e(EXPR_PARAM, 0, 0);
        e.param_index = index;
        return e;
    }
};
struct abs_ {
    expr_ e;
    abs_(const expr_& e) : e(e) {}
    operator expr_() const { return expr_(EXPR_ABS, new expr_(e), 0); }
};

class AnimatorEd;
class animBtNode {
protected:
    void updatePriority(int prio) {
        if (priority < prio) {
            priority = prio;
        }
    }
public:
    int priority = 0;

    virtual ~animBtNode() {}
    virtual bool compile(sklSkeletonEditable* skl, std::set<animBtNode*>& exec_set, int order) = 0;
    virtual void update(AnimatorEd* animator, float dt) = 0;
    virtual animSampleBuffer* getOutputSamples() = 0;
};
class animBtNodeClip : public animBtNode {
    RHSHARED<Animation> anim;
    animSampler         sampler;
    animSampleBuffer    samples;
    float cursor = 0.0f;
public:
    void setAnimation(const RHSHARED<Animation>& anim) { this->anim = anim; }

    bool compile(sklSkeletonEditable* skl, std::set<animBtNode*>& exec_set, int order) override {
        if (!anim) {
            LOG_ERR("animBtNodeClip::compile(): animation is null");
            assert(false);
            return false;
        }
        updatePriority(order);
        sampler = animSampler(skl, anim.get());
        samples.init(skl);
        exec_set.insert(this);
        return true;
    }
    void update(AnimatorEd* animator, float dt) override {
        if (cursor > anim->length) {
            cursor -= anim->length;
        }
        sampler.sample(samples.data(), samples.count(), cursor);
        cursor += dt * anim->fps;
    }
    animSampleBuffer* getOutputSamples() override {
        return &samples;
    }
};
class animBtNodeFrame : public animBtNode {
    animSampleBuffer samples;
public:
    RHSHARED<Animator>  anim;
    int                 keyframe;
    bool compile(sklSkeletonEditable* skl, std::set<animBtNode*>& exec_set, int order) override {
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
    void update(AnimatorEd* animator, float dt) override {
        float weight = weight_expression.evaluate(animator).to_float();
        animBlendSamples(*in_a->getOutputSamples(), *in_b->getOutputSamples(), samples, weight);
    }
    animSampleBuffer* getOutputSamples() override {
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
            return a->priority > b->priority;
        });
        for (int i = 0; i < exec_chain.size(); ++i) {
            LOG_DBG(exec_chain[i]->priority);
        }
        return true;
    }
    void update(AnimatorEd* animator, animSampleBuffer* samples, float dt) override {
        for (int i = 0; i < exec_chain.size(); ++i) {
            auto node = exec_chain[i];
            node->update(animator, dt);
        }
        samples->copy(*out_node->getOutputSamples());
    }
};

#include "game/animator/exp_animation_language.hpp"
class AnimatorEd {
    RHSHARED<sklSkeletonEditable> skeleton;
    std::unique_ptr<animUnit> rootUnit;
    animSampleBuffer samples;

    std::unordered_map<int, float>          parameters;
    std::unordered_map<std::string, int>    param_names;
    // TODO: Input Events (signals?) 
    // TODO: Output events?
    // TODO: Output values?

public:
    AnimatorEd() {
        
    }

    /// Edit-time
    void setSkeleton(RHSHARED<sklSkeletonEditable> skl) {
        skeleton = skl;
        samples.init(skeleton.get());
    }
    template<typename T>
    T* setRoot() {
        auto ptr = new T();
        rootUnit.reset(ptr);
        return ptr;
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
            assert(it != parameters.end());
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
        rootUnit->compile(skeleton.get());
        return true;
    }

    /// Runtime
    void update(float dt) {
        if (!rootUnit) {
            return;
        }
        rootUnit->update(this, &samples, dt);
    }

    animSampleBuffer* getSampleBuffer() {
        return &samples;
    }
};
inline param_::param_(AnimatorEd* animator, const std::string& name) {
    index = animator->getParamId(name.c_str());
}
inline value_ expr_::evaluate(AnimatorEd* animator) const {
    value_ v;
    value_ l;
    value_ r;
    switch (type) {
    case EXPR_EQ:
        l = left->evaluate(animator);
        r = right->evaluate(animator);
        if (l.type != r.type) {
            r.convert(l.type, r);
        }
        switch (l.type) {
        case VALUE_FLOAT: return value_(l.float_ == r.float_);
        case VALUE_FLOAT2: return value_(l.float2.x == r.float2.x && l.float2.y == r.float2.y);
        case VALUE_FLOAT3: return value_(l.float3.x == r.float3.x && l.float3.y == r.float3.y && l.float3.z == r.float3.z);
        case VALUE_INT: return value_(l.int_ == r.int_);
        case VALUE_INT2: return value_(l.int2.x == r.int2.x && l.int2.y == r.int2.y);
        case VALUE_INT3: return value_(l.int3.x == r.int3.x && l.int3.y == r.int3.y && l.int3.z == r.int3.z);
        case VALUE_BOOL: return value_(l.bool_ == r.bool_);
        default: assert(false); return value_(false);
        }
        break;
    case EXPR_NOTEQ:
        l = left->evaluate(animator);
        r = right->evaluate(animator);
        if (l.type != r.type) {
            r.convert(l.type, r);
        }
        switch (l.type) {
        case VALUE_FLOAT: return value_(l.float_ != r.float_);
        case VALUE_FLOAT2: return value_(l.float2.x != r.float2.x || l.float2.y != r.float2.y);
        case VALUE_FLOAT3: return value_(l.float3.x != r.float3.x || l.float3.y != r.float3.y || l.float3.z != r.float3.z);
        case VALUE_INT: return value_(l.int_ != r.int_);
        case VALUE_INT2: return value_(l.int2.x != r.int2.x || l.int2.y != r.int2.y);
        case VALUE_INT3: return value_(l.int3.x != r.int3.x || l.int3.y != r.int3.y || l.int3.z != r.int3.z);
        case VALUE_BOOL: return value_(l.bool_ != r.bool_);
        default: assert(false); return value_(false);
        }
        break;
    case EXPR_LESS:
        l = left->evaluate(animator);
        r = right->evaluate(animator);
        if (l.type != r.type) {
            r.convert(l.type, r);
        }
        switch (l.type) {
        case VALUE_FLOAT: return value_(l.float_ < r.float_);
        case VALUE_INT: return value_(l.int_ < r.int_);
        case VALUE_BOOL: return value_(l.bool_ < r.bool_);
        default: assert(false); return value_(false);
        }
        break;
    case EXPR_MORE:
        l = left->evaluate(animator);
        r = right->evaluate(animator);
        if (l.type != r.type) {
            r.convert(l.type, r);
        }
        switch (l.type) {
        case VALUE_FLOAT: return value_(l.float_ > r.float_);
        case VALUE_INT: return value_(l.int_ > r.int_);
        case VALUE_BOOL: return value_(l.bool_ > r.bool_);
        default: assert(false); return value_(false);
        }
        break;
    case EXPR_LESSEQ:
        l = left->evaluate(animator);
        r = right->evaluate(animator);
        if (l.type != r.type) {
            r.convert(l.type, r);
        }
        switch (l.type) {
        case VALUE_FLOAT: return value_(l.float_ <= r.float_);
        case VALUE_INT: return value_(l.int_ <= r.int_);
        case VALUE_BOOL: return value_(l.bool_ <= r.bool_);
        default: assert(false); return value_(false);
        }
        break;
    case EXPR_MOREEQ:
        l = left->evaluate(animator);
        r = right->evaluate(animator);
        if (l.type != r.type) {
            r.convert(l.type, r);
        }
        switch (l.type) {
        case VALUE_FLOAT: return value_(l.float_ >= r.float_);
        case VALUE_INT: return value_(l.int_ >= r.int_);
        case VALUE_BOOL: return value_(l.bool_ >= r.bool_);
        default: assert(false); return value_(false);
        }
        break;
    case EXPR_ADD:
        l = left->evaluate(animator);
        r = right->evaluate(animator);
        if (l.type != r.type) {
            r.convert(l.type, r);
        }
        switch (l.type) {
        case VALUE_FLOAT: return value_(l.float_ + r.float_);
        case VALUE_FLOAT2: return value_(l.float2 + r.float2);
        case VALUE_FLOAT3: return value_(l.float3 + r.float3);
        case VALUE_INT: return value_(l.int_ + r.int_);
        case VALUE_INT2: return value_(l.int2 + r.int2);
        case VALUE_INT3: return value_(l.int3 + r.int3);
        case VALUE_BOOL: return value_(l.bool_ + r.bool_);
        default: assert(false); return value_(false);
        }
        break;
    case EXPR_SUB:
        l = left->evaluate(animator);
        r = right->evaluate(animator);
        if (l.type != r.type) {
            r.convert(l.type, r);
        }
        switch (l.type) {
        case VALUE_FLOAT: return value_(l.float_ - r.float_);
        case VALUE_FLOAT2: return value_(l.float2 - r.float2);
        case VALUE_FLOAT3: return value_(l.float3 - r.float3);
        case VALUE_INT: return value_(l.int_ - r.int_);
        case VALUE_INT2: return value_(l.int2 - r.int2);
        case VALUE_INT3: return value_(l.int3 - r.int3);
        case VALUE_BOOL: return value_(l.bool_ - r.bool_);
        default: assert(false); return value_(false);
        }
        break;
    case EXPR_MUL:
        l = left->evaluate(animator);
        r = right->evaluate(animator);
        if (l.type != r.type) {
            r.convert(l.type, r);
        }
        switch (l.type) {
        case VALUE_FLOAT: return value_(l.float_ * r.float_);
            //case VALUE_FLOAT2: return value_(l.float2 * r.float2);
            //case VALUE_FLOAT3: return value_(l.float3 * r.float3);
        case VALUE_INT: return value_(l.int_ * r.int_);
            //case VALUE_INT2: return value_(l.int2 * r.int2);
            //case VALUE_INT3: return value_(l.int3 * r.int3);
        case VALUE_BOOL: return value_(l.bool_ * r.bool_);
        default: assert(false); return value_(false);
        }
        break;
    case EXPR_DIV:
        l = left->evaluate(animator);
        r = right->evaluate(animator);
        if (l.type != r.type) {
            r.convert(l.type, r);
        }
        switch (l.type) {
        case VALUE_FLOAT: return value_(l.float_ / r.float_);
            //case VALUE_FLOAT2: return value_(l.float2 / r.float2);
            //case VALUE_FLOAT3: return value_(l.float3 / r.float3);
        case VALUE_INT: return value_(l.int_ / r.int_);
            //case VALUE_INT2: return value_(l.int2 / r.int2);
            //case VALUE_INT3: return value_(l.int3 / r.int3);
        case VALUE_BOOL: return value_(l.bool_ / r.bool_);
        default: assert(false); return value_(false);
        }
        break;
    case EXPR_MOD:
        l = left->evaluate(animator);
        r = right->evaluate(animator);
        if (l.type != r.type) {
            r.convert(l.type, r);
        }
        switch (l.type) {
        case VALUE_FLOAT: return value_(fmod(l.float_, r.float_));
            //case VALUE_FLOAT2: return value_(l.float2 / r.float2);
            //case VALUE_FLOAT3: return value_(l.float3 / r.float3);
        case VALUE_INT: return value_(l.int_ % r.int_);
            //case VALUE_INT2: return value_(l.int2 / r.int2);
            //case VALUE_INT3: return value_(l.int3 / r.int3);
        case VALUE_BOOL: return value_(l.bool_ % r.bool_);
        default: assert(false); return value_(false);
        }
        break;
    case EXPR_ABS:
        l = left->evaluate(animator);
        switch (l.type) {
        case VALUE_FLOAT: return value_(fabs(l.float_));
        case VALUE_INT: return value_(abs(l.int_));
        default: assert(false); return value_(false);
        }
        break;
    case EXPR_LIT_FLOAT:
        return value_(float_value);
        break;
    case EXPR_LIT_INT:
        return value_(int_value);
        break;
    case EXPR_LIT_BOOL:
        return value_(bool_value);
        break;
    case EXPR_PARAM:
        return value_(animator->getParamValue(param_index));
        break;
    case EXPR_EVENT:
        return value_(false); // TODO
        break;
    default:
        assert(false);
    }
}
inline std::string expr_::toString(AnimatorEd* animator) const {
    switch (type) {
    case EXPR_EQ:
        return MKSTR("(" << left->toString(animator) << " == " << right->toString(animator) << ")");
        break;
    case EXPR_NOTEQ:
        return MKSTR("(" << left->toString(animator) << " != " << right->toString(animator) << ")");
        break;
    case EXPR_LESS:
        return MKSTR("(" << left->toString(animator) << " < " << right->toString(animator) << ")");
        break;
    case EXPR_MORE:
        return MKSTR("(" << left->toString(animator) << " > " << right->toString(animator) << ")");
        break;
    case EXPR_LESSEQ:
        return MKSTR("(" << left->toString(animator) << " <= " << right->toString(animator) << ")");
        break;
    case EXPR_MOREEQ:
        return MKSTR("(" << left->toString(animator) << " >= " << right->toString(animator) << ")");
        break;
    case EXPR_ADD:
        return MKSTR("(" << left->toString(animator) << " + " << right->toString(animator) << ")");
        break;
    case EXPR_SUB:
        return MKSTR("(" << left->toString(animator) << " - " << right->toString(animator) << ")");
        break;
    case EXPR_MUL:
        return MKSTR("(" << left->toString(animator) << " * " << right->toString(animator) << ")");
        break;
    case EXPR_DIV:
        return MKSTR("(" << left->toString(animator) << " / " << right->toString(animator) << ")");
        break;
    case EXPR_MOD:
        return MKSTR("(" << left->toString(animator) << " % " << right->toString(animator) << ")");
        break;
    case EXPR_ABS:
        return MKSTR("abs(" << left->toString(animator) << ")");
        break;
    case EXPR_LIT_FLOAT:
        return MKSTR(float_value);
        break;
    case EXPR_LIT_INT:
        return MKSTR(int_value);
        break;
    case EXPR_LIT_BOOL:
        return MKSTR(bool_value);
        break;
    case EXPR_PARAM:
        return MKSTR(animator->getParamValue(param_index));
        break;
    case EXPR_EVENT:
        return "event";
        break;
    default:
        assert(false);
    }
}


class actorAnimTest : public wActor {
    HSHARED<sklmSkeletalModelInstance> model_inst;
    AnimatorEd animator;
public:
    actorAnimTest() {
        {
            expr_ e = (param_(&animator, "velocity") + 10.0f) % 9.f;
            e.dbgLog(&animator);
            value_ v = e.evaluate(&animator);
            v.dbgLog();
        }

        auto model = resGet<sklmSkeletalModelEditable>("models/chara_24/chara_24.skeletal_model");
        model_inst = model->createInstance();
        model_inst->getSkeletonInstance()->getWorldTransformsPtr()[0] = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(4, 0, 0));
        
        {
            animator.setSkeleton(model->getSkeleton());
            animator.addParam("velocity");
            animator.addParam("test");

            auto bt = animator.setRoot<animUnitBlendTree>();
            auto node_clip = bt->addNode<animBtNodeClip>();
            auto node_clip2 = bt->addNode<animBtNodeClip>();
            auto node_blend2 = bt->addNode<animBtNodeBlend2>();
            node_clip->setAnimation(resGet<Animation>("models/chara_24/Idle.animation"));
            node_clip2->setAnimation(resGet<Animation>("models/chara_24/Run.animation"));
            node_blend2->setInputs(node_clip, node_clip2);
            node_blend2->setWeightExpression(
                abs_(1.0f - param_(&animator, "velocity") % 2.0f)
            );
            bt->setOutputNode(node_blend2);

            /*
            auto fsm = animator.setRoot<animUnitFsm>();
            
            auto state = fsm->addState("stateA");
            auto single = state->setUnit<animUnitSingle>();
            single->setAnimation(resGet<Animation>("models/anim_test/test.animation"));

            auto state2 = fsm->addState("stateB");
            auto single2 = state2->setUnit<animUnitSingle>();
            single2->setAnimation(resGet<Animation>("models/anim_test/test2.animation"));

            fsm->addTransition(
                "stateA", "stateB",
                cond_anim_end(), 0.2f
            );
            fsm->addTransition(
                "stateB", "stateA",
                cond_anim_end(), 0.2f
            );*/

            animator.compile();
        }
    }

    void onSpawn(wWorld* world) override {
        model_inst->onSpawn(world->getRenderScene());
    }
    void onDespawn(wWorld* world) override {
        model_inst->onDespawn(world->getRenderScene());
    }

    void update(float dt) {
        static float velocity = .0f;
        velocity += dt;
        animator.setParamValue(animator.getParamId("velocity"), velocity);

        animator.update(dt);
        animator.getSampleBuffer()->applySamples(model_inst->getSkeletonInstance());
    }
};

class actorUltimaWeapon : public wActor {
    HSHARED<sklmSkeletalModelInstance> model_inst;
    AnimatorEd animator;
public:
    actorUltimaWeapon() {
        auto model = resGet<sklmSkeletalModelEditable>("models/ultima_weapon/ultima_weapon.skeletal_model");
        model_inst = model->createInstance();
        model_inst->getSkeletonInstance()->getWorldTransformsPtr()[0]
            = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(6, 0, -6))
            * gfxm::scale(gfxm::mat4(1.0f), gfxm::vec3(2, 2, 2));

        animator.setSkeleton(model->getSkeleton());
        auto single = animator.setRoot<animUnitSingle>();
        single->setAnimation(resGet<Animation>("models/ultima_weapon/Idle.animation"));
        animator.compile();
    }
    void onSpawn(wWorld* world) override {
        model_inst->onSpawn(world->getRenderScene());
    }
    void onDespawn(wWorld* world) override {
        model_inst->onDespawn(world->getRenderScene());
    }
    void update(float dt) {
        animator.update(dt);
        animator.getSampleBuffer()->applySamples(model_inst->getSkeletonInstance());
    }
};

class Door : public wActor {
    HSHARED<sklmSkeletalModelInstance> model_inst;

    HSHARED<Animation>  anim_open;
    animSampler    anim_sampler;
    animSampleBuffer    samples;

    CollisionSphereShape     shape_sphere;
    Collider                 collider_beacon;

    bool is_opening = false;
    float anim_cursor = .0f;
public:
    Actor ref_point_front;
    Actor ref_point_back;

    Door() {
        auto model = resGet<sklmSkeletalModelEditable>("models/door/door.skeletal_model");
        model_inst = model->createInstance();

        anim_open = resGet<Animation>("models/door/Open.animation");
        anim_sampler = animSampler(model->getSkeleton().get(), anim_open.get());
        samples.init(model->getSkeleton().get());

        setTranslation(gfxm::vec3(1, 0, 6.0f));

        model_inst->getSkeletonInstance()->getWorldTransformsPtr()[0]
            = getWorldTransform();

        shape_sphere.radius = 0.1f;
        collider_beacon.setShape(&shape_sphere);        
        collider_beacon.position = getTranslation();
        collider_beacon.setUserPtr(this);

        // Ref points for the character to adjust to for door opening animations
        gfxm::vec3 door_pos = getTranslation();
        door_pos.y = .0f;
        ref_point_front.setTranslation(door_pos + gfxm::vec3(0, 0, 1));
        ref_point_front.setRotation(gfxm::angle_axis(gfxm::pi, gfxm::vec3(0, 1, 0)));
        ref_point_back.setTranslation(door_pos + gfxm::vec3(0, 0, -1));
    }

    void onSpawn(wWorld* world) override {
        model_inst->onSpawn(world->getRenderScene());

        world->getCollisionWorld()->addCollider(&collider_beacon);
    }
    void onDespawn(wWorld* world) override {
        model_inst->onDespawn(world->getRenderScene());

        world->getCollisionWorld()->removeCollider(&collider_beacon);
    }

    void update(float dt) {
        if (is_opening) {            
            anim_sampler.sample(samples.data(), samples.count(), anim_cursor * anim_open->fps);
            
            for (int i = 1; i < samples.count(); ++i) {
                auto& s = samples[i];
                gfxm::mat4 m = gfxm::translate(gfxm::mat4(1.0f), s.t)
                    * gfxm::to_mat4(s.r)
                    * gfxm::scale(gfxm::mat4(1.0f), s.s);
                model_inst->getSkeletonInstance()->getLocalTransformsPtr()[i] = m;
            }
            anim_cursor += dt;
            if (anim_cursor >= anim_open->length) {
                is_opening = false;
            }
        }
    }

    wRsp onMessage(wMsg msg) override {
        if (auto m = wMsgTranslate<wMsgDoorOpen>(msg)) {
            LOG_DBG("Door: Open message received");
            is_opening = true;
            anim_cursor = .0f;
        } else {
            LOG_DBG("Door: unknown message received");
        }
        return 0;
    }
};


enum class CHARA_ANIM_STATE {
    LOCOMOTION,
    DOOR_OPEN
};
struct CharacterAnimContext {
    float velocity = .0f;
    bool evtDoorOpen = false;
    bool out_evtDoorOpenDone = false;
};
class CharacterAnimState;
class CharacterAnimStateSwitchCondition {
public:
    CharacterAnimState* target = 0;
    virtual ~CharacterAnimStateSwitchCondition() {}
    virtual bool check(CharacterAnimContext& ctx) = 0;
};
class CharacterAnimStateSwitchConditionToDoorOpen : public CharacterAnimStateSwitchCondition {
public:
    bool check(CharacterAnimContext& ctx) override {
        return ctx.evtDoorOpen;
    }
};
class CharacterAnimStateSwitchConditionDoorOpenToLoco : public CharacterAnimStateSwitchCondition {
    bool check(CharacterAnimContext& ctx) override {
        return ctx.out_evtDoorOpenDone;
    }
};

class CharacterAnimState {
public:
    std::vector<CharacterAnimStateSwitchCondition*> conditions;

    virtual ~CharacterAnimState() {}
    virtual void onStart() = 0;
    virtual void onUpdate(CharacterAnimContext& ctx, float dt, AnimSample* out_samples, int max_samples, AnimSample* out_root_motion) = 0;
};
class CharacterAnimStateOneShot : public CharacterAnimState {
public:
    animSampler sampler;
    float cursor_normal = .0f;

    void onStart() override {
        cursor_normal = .0f;
    }
    void onUpdate(CharacterAnimContext& ctx, float dt, AnimSample* out_samples, int max_samples, AnimSample* out_root_motion) override {
        float cursor_normal_prev = cursor_normal;
        cursor_normal += (sampler.getAnimation()->fps / sampler.getAnimation()->length) * dt;
        sampler.sample_normalized(out_samples, max_samples, cursor_normal);

        if (sampler.getAnimation()->hasRootMotion()) {
            sampler.getAnimation()->sample_root_motion(out_root_motion, cursor_normal_prev * sampler.getAnimation()->length, cursor_normal * sampler.getAnimation()->length);
        }

        if (cursor_normal >= 1.0f) {
            ctx.out_evtDoorOpenDone = true;
        }
    }
};
class CharacterAnimStateBlend2 : public CharacterAnimState {
public:
    animSampler sampler_a;
    animSampler sampler_b;
    float cursor_normal = .0f;
    float weight = .0f;

    void onStart() override {
        cursor_normal = .0f;
    }
    void onUpdate(CharacterAnimContext& ctx, float dt, AnimSample* out_samples, int max_samples, AnimSample* out_root_motion) override {
        weight = ctx.velocity;

        std::vector<AnimSample> samples_a
            = std::vector<AnimSample>(max_samples);
        std::vector<AnimSample> samples_b
            = std::vector<AnimSample>(max_samples);

        sampler_a.sample_normalized(samples_a.data(), samples_a.size(), cursor_normal);
        sampler_b.sample_normalized(samples_b.data(), samples_b.size(), cursor_normal);

        for (int i = 1; i < max_samples; ++i) {
            gfxm::vec3 tr = gfxm::lerp(samples_a[i].t, samples_b[i].t, weight);
            gfxm::quat rot = gfxm::slerp(samples_a[i].r, samples_b[i].r, weight);
            gfxm::vec3 scl = gfxm::lerp(samples_a[i].s, samples_b[i].s, weight);
            gfxm::mat4 m = gfxm::translate(gfxm::mat4(1.0f), tr)
                * gfxm::to_mat4(rot)
                * gfxm::scale(gfxm::mat4(1.0f), scl);
            AnimSample sample;
            sample.t = tr;
            sample.r = rot;
            sample.s = scl;
            out_samples[i] = sample;
        }

        float cursor_step_a = sampler_a.getAnimation()->fps / sampler_a.getAnimation()->length;
        float cursor_step_b = sampler_b.getAnimation()->fps / sampler_b.getAnimation()->length;
        float cursor_step = gfxm::lerp(cursor_step_a, cursor_step_b, weight);
        cursor_normal += cursor_step * dt;
        if (cursor_normal > 1.0f) {
            cursor_normal = cursor_normal - 1.0f;
        }
    }
};
#include "skeleton/skeleton_editable.hpp"
#include "skeleton/skeleton_instance.hpp"
class CharacterAnimator {
    float cursor_normal = .0f;
    sklSkeletonEditable* skeleton = 0;

    CHARA_ANIM_STATE state = CHARA_ANIM_STATE::LOCOMOTION;
public:
    animSampler anim_idle;
    animSampler anim_run;
    animSampler anim_door_open;

    CharacterAnimContext context;

    CharacterAnimState* current_state = 0;
    CharacterAnimStateOneShot state_door_open;
    CharacterAnimStateBlend2 state_locomotion;

    CharacterAnimStateSwitchConditionToDoorOpen cond_to_door_open;
    CharacterAnimStateSwitchConditionDoorOpenToLoco cond_door_open_to_loco;

    std::vector<AnimSample> out_samples;
    AnimSample out_root_motion;

    void setSkeleton(sklSkeletonEditable* sk) {
        skeleton = sk;
        out_samples.resize(skeleton->boneCount());
    }

    void init() {
        context.evtDoorOpen = false;
        context.out_evtDoorOpenDone = false;
        context.velocity = .0f;

        cond_to_door_open.target = &state_door_open;
        cond_door_open_to_loco.target = &state_locomotion;

        state_locomotion.sampler_a = anim_idle;
        state_locomotion.sampler_b = anim_run;
        state_door_open.sampler = anim_door_open;

        state_locomotion.conditions.push_back(&cond_to_door_open);
        state_door_open.conditions.push_back(&cond_door_open_to_loco);

        current_state = &state_locomotion;
    }

    void update(float dt) {
        // Clear out-event notifiers
        context.out_evtDoorOpenDone = false;

        // Clear root motion data
        out_root_motion.t = gfxm::vec3(0, 0, 0);
        out_root_motion.r = gfxm::quat(0, 0, 0, 1);
        out_root_motion.s = gfxm::vec3(0, 0, 0);
        
        
        current_state->onUpdate(context, dt, out_samples.data(), out_samples.size(), &out_root_motion);
        for (int i = 0; i < current_state->conditions.size(); ++i) {
            if (current_state->conditions[i]->check(context)) {
                current_state = current_state->conditions[i]->target;
                current_state->onStart();
                break;
            }
        }

        // clear event flags
        context.evtDoorOpen = false;
    }
};


struct ColliderData {
    Actor*          actor;
    gfxm::vec3      offset;
    CollisionShape* shape;
    Collider*       collider;
};
struct ColliderProbeData {
    Actor*          actor;
    gfxm::vec3      offset;
    CollisionShape* shape;
    ColliderProbe*  collider_probe;
};

enum class CHARACTER_STATE {
    LOCOMOTION,
    DOOR_OPEN
};


#include "import/assimp_load_skeletal_model.hpp"
class Character : public wActor {
    struct {
        RHSHARED<sklmSkeletalModelEditable> model;
        HSHARED<sklmSkeletalModelInstance>  model_inst;
    };

    std::unique_ptr<scnDecal> decal;
    std::unique_ptr<scnTextBillboard> name_caption;
    scnNode caption_node;
    // TEXT STUFF, MUST BE SHARED
    Typeface typeface;
    std::unique_ptr<Font> font;

    // Anim
    CharacterAnimator animator;
    HSHARED<Animation> anim_idle;
    HSHARED<Animation> anim_run;
    HSHARED<Animation> anim_door_open;

    // Gameplay
    wActor* targeted_actor = 0;
    CHARACTER_STATE state = CHARACTER_STATE::LOCOMOTION;

    gfxm::vec3 forward_vec = gfxm::vec3(0, 0, 1);
    gfxm::vec3 loco_vec_tgt;
    gfxm::vec3 loco_vec;
    float velocity = .0f;

    // Collision
    CollisionCapsuleShape    shape_capsule;
    Collider                 collider;
    CollisionSphereShape     shape_sphere;
    ColliderProbe            collider_probe;
public:
    Character() {
        {
            model.reset(HANDLE_MGR<sklmSkeletalModelEditable>::acquire());
            assimpImporter importer;
            importer.loadFile("chara_24.fbx");
            importer.loadSkeletalModel(model.get());
            model_inst = model->createInstance();
        }

        decal.reset(new scnDecal);
        decal->setTexture(resGet<gpuTexture2d>("images/character_selection_decal.png"));
        decal->setBoxSize(1.3f, 1.0f, 1.3f);
        decal->setBlending(GPU_BLEND_MODE::NORMAL);
        decal->setSkeletonNode(model_inst->getSkeletonInstance()->getScnSkeleton(), 0);

        typefaceLoad(&typeface, "OpenSans-Regular.ttf");
        font.reset(new Font(&typeface, 16, 72));
        name_caption.reset(new scnTextBillboard(font.get()));
        name_caption->setSkeletonNode(model_inst->getSkeletonInstance()->getScnSkeleton(), 16);
        caption_node.local_transform = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(.0f, 1.9f, .0f));
        caption_node.attachToSkeleton(model_inst->getSkeletonInstance()->getScnSkeleton(), 0);
        name_caption->setNode(&caption_node);

        anim_idle = resGet<Animation>("chara_24/Idle.sanim");
        anim_run = resGet<Animation>("chara_24/Run.sanim");
        anim_door_open = resGet<Animation>("models/chara_24_anim_door/Action_OpenDoor.animation");

        animator.anim_idle = animSampler(model->getSkeleton().get(), anim_idle.get());
        animator.anim_run = animSampler(model->getSkeleton().get(), anim_run.get());
        animator.anim_door_open = animSampler(model->getSkeleton().get(), anim_door_open.get());
        animator.setSkeleton(model->getSkeleton().get());
        animator.init();
        
        // Collision
        shape_capsule.radius = 0.3f;
        collider.setShape(&shape_capsule);        

        shape_sphere.radius = 0.85f;
        collider_probe.setShape(&shape_sphere);        
    }

    void setDesiredLocomotionVector(const gfxm::vec3& loco) {
        float len = loco.length();
        gfxm::vec3 norm = len > 1.0f ? gfxm::normalize(loco) : loco;
        if (loco.length() > FLT_EPSILON) {
            forward_vec = gfxm::normalize(loco);
        }
        loco_vec_tgt = norm;
    }
    void actionUse() {
        if (targeted_actor) {
            targeted_actor->sendMessage(wMsgMake(wMsgDoorOpen()));

            animator.context.evtDoorOpen = true;
            state = CHARACTER_STATE::DOOR_OPEN;
            velocity = .0f;
            loco_vec = gfxm::vec3(0, 0, 0);
            /*
            Door* door = dynamic_cast<Door*>(targeted_actor);
            if (door) {
                // Pick a starting point for door opening animation
                gfxm::vec3 door_pos = door->getWorldTransform() * gfxm::vec4(0, 0, 0, 1);
                gfxm::vec3 char_pos = getWorldTransform() * gfxm::vec4(0, 0, 0, 1);
                gfxm::vec3 door_char_norm = gfxm::normalize(char_pos - door_pos);
                gfxm::vec3 door_norm = gfxm::normalize(door->getWorldTransform() * gfxm::vec4(0, 0, -1, 0));
                float dot = gfxm::dot(door_norm, door_char_norm);
                if (dot > .0f) {
                    setTranslation(door->ref_point_back.getTranslation());
                    setRotation(door->ref_point_back.getRotation());
                } else {
                    setTranslation(door->ref_point_front.getTranslation());
                    setRotation(door->ref_point_front.getRotation());
                }

                animator.context.evtDoorOpen = true;
                state = CHARACTER_STATE::DOOR_OPEN;
                velocity = .0f;
                loco_vec = gfxm::vec3(0, 0, 0);
                LOG_WARN("Activated a door!");
            }*/
        }
    }

    void update_locomotion(float dt) {
        // Handle input
        velocity = gfxm::lerp(velocity, loco_vec.length(), 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f));
        loco_vec = gfxm::lerp(loco_vec, loco_vec_tgt, 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f));
        if (velocity > FLT_EPSILON) {
            translate(loco_vec * dt * 5.0f);

            gfxm::mat3 orient;
            orient[2] = forward_vec;
            orient[1] = gfxm::vec3(0, 1, 0);
            orient[0] = gfxm::cross(orient[1], orient[2]);
            setRotation(gfxm::slerp(getRotation(), gfxm::to_quat(orient), 1 - pow(1 - 0.1f * 3.0f, dt * 60.0f)));
        }

        // Choose an actionable object if there are any available
        for (int i = 0; i < collider_probe.overlappingColliderCount(); ++i) {
            Collider* other = collider_probe.getOverlappingCollider(i);
            void* user_ptr = other->getUserPtr();
            if (user_ptr) {
                targeted_actor = (wActor*)user_ptr;
                break;
            }
        }
    }
    void update_doorOpen(float dt) {
        if (animator.context.out_evtDoorOpenDone) {
            state = CHARACTER_STATE::LOCOMOTION;
            forward_vec = getWorldTransform() * gfxm::vec4(0, 0, 1, 0);
        }
    }
    void update(float dt) {
        // Clear stuff
        targeted_actor = 0;

        switch (state) {
        case CHARACTER_STATE::LOCOMOTION:
            update_locomotion(dt);
            break;
        case CHARACTER_STATE::DOOR_OPEN:
            update_doorOpen(dt);
            break;
        }

        // Apply animations and skinning
        animator.context.velocity = velocity;
        animator.update(dt);
        for (int i = 1; i < animator.out_samples.size(); ++i) {
            auto& s = animator.out_samples[i];
            gfxm::mat4 m = gfxm::translate(gfxm::mat4(1.0f), s.t)
                * gfxm::to_mat4(s.r)
                * gfxm::scale(gfxm::mat4(1.0f), s.s);
            model_inst->getSkeletonInstance()->getLocalTransformsPtr()[i] = m;
        }

        // Apply root motion
        translate(gfxm::vec3(getWorldTransform() * gfxm::vec4(animator.out_root_motion.t, .0f)));

        // Update transforms
        model_inst->getSkeletonInstance()->getWorldTransformsPtr()[0] = getWorldTransform();

        collider.position = getTranslation() + gfxm::vec3(0, 1.0f, 0);
        collider.rotation = getRotation();
        collider_probe.position = getWorldTransform() * gfxm::vec4(0, 0.5f, 0.64f, 1.0f);
        collider_probe.rotation = gfxm::to_quat(gfxm::to_orient_mat3(getWorldTransform()));
    }

    void onSpawn(wWorld* world) override {
        model_inst->onSpawn(world->getRenderScene());

        world->getRenderScene()->addRenderObject(decal.get());

        world->getRenderScene()->addNode(&caption_node);
        world->getRenderScene()->addRenderObject(name_caption.get());

        world->getCollisionWorld()->addCollider(&collider);
        world->getCollisionWorld()->addCollider(&collider_probe);
    }
    void onDespawn(wWorld* world) override {
        model_inst->onDespawn(world->getRenderScene());

        world->getRenderScene()->removeRenderObject(decal.get());

        world->getRenderScene()->removeNode(&caption_node);
        world->getRenderScene()->removeRenderObject(name_caption.get());

        world->getCollisionWorld()->removeCollider(&collider);
        world->getCollisionWorld()->removeCollider(&collider_probe);
    }
};