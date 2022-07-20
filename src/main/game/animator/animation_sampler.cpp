#include "animation_sampler.hpp"

#include "assimp_load_scene.hpp"


AnimationSampler::AnimationSampler(sklSkeletonEditable* skeleton, Animation* anim) {
    assert(anim);
    assert(skeleton);
    animation = anim;
    mapping = std::vector<int32_t>(anim->nodeCount(), -1);
    const auto& bone_array = skeleton->getBoneArray();
    for (auto& it : bone_array) {
        std::string bone_name = it->getName();
        int transform_index = it->getIndex();
        int anim_node_index = anim->getNodeIndex(bone_name);
        if (anim_node_index < 0) continue;
        mapping[anim_node_index] = transform_index;
    }
}

void AnimationSampler::sample(AnimSample* out_samples, int sample_count, float cursor) {
    assert(animation);
    animation->sample_remapped(out_samples, sample_count, cursor, mapping);
}
void AnimationSampler::sample_normalized(AnimSample* out_samples, int sample_count, float cursor_normal) {
    assert(animation);
    animation->sample_remapped(out_samples, sample_count, cursor_normal * animation->length, mapping);
}

Animation* AnimationSampler::getAnimation() {
    return animation;
}