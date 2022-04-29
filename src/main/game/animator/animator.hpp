#pragma once

#include "assimp_load_scene.hpp"
#include "common/animation/animation.hpp"

#include "game/animator/animation_sampler.hpp"

struct AnimatorData {
    Skeleton* skeleton;
    std::vector<Animation*> animations;
    std::vector<std::vector<int>> mappings;
};


class AnimState {
public:
    virtual AnimState* onUpdate(float dt) = 0;
};

class AnimStateOneShot : public AnimState {
    float cursor_normal = .0f;
public:
    AnimationSampler anim;

    AnimState* onUpdate(float dt) override {


        cursor_normal += dt * (anim.getAnimation()->fps / anim.getAnimation()->length);
        if (cursor_normal >= 1.0f) {
            // TODO Set a flag indicating completion
            // return state to switch to
        }
        return this;
    }
};
class AnimStateBlend2 : public AnimState {
    float cursor_normal = .0f;
public:
    AnimationSampler anim_a;
    AnimationSampler anim_b;
    float weight = .0f;

    AnimState* onUpdate(float dt) override {
        
        
    }
};

class AnimProgram {
public:
    virtual ~AnimProgram() {}

    virtual void onUpdate(float dt) = 0;
};

class AnimFSM : public AnimProgram {
    AnimState* current_state = 0;
public:
    void onUpdate(float dt) override {

    }
};

struct AnimatorContext {
    std::map<std::string, AnimationSampler> samplers;

};

class Animator {
    AnimatorContext context;
public:
    void update(float dt) {

    }
};