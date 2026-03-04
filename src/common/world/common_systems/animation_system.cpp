#include "animation_system.hpp"



void AnimationSystem::addAnimObject(AnimObject* o) {
    objects.insert(o);
}

void AnimationSystem::removeAnimObject(AnimObject* o) {
    objects.erase(o);
}

void AnimationSystem::update(float dt) {
    for (auto o : objects) {
        auto& anim_inst = o->anim_inst;
        anim_inst->update(dt);

        auto& skl_inst = o->skl_inst;
        anim_inst->getSampleBuffer()->applySamples(skl_inst.get());
        anim_inst->getAudioCmdBuffer()->execute(skl_inst.get());

        // Root motion
        /*
        if (auto root = getRoot()) {
            gfxm::vec3 rm_t = gfxm::vec3(root->getWorldTransform() * gfxm::vec4(anim_inst->getSampleBuffer()->getRootMotionSample().t, .0f));
            rm_t.y = .0f;
            root->translate(rm_t);
            root->rotate(anim_inst->getSampleBuffer()->getRootMotionSample().r);
        }*/
    }
}