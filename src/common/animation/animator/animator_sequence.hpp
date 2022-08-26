#pragma once

#include <unordered_map>
#include "reflection/reflection.hpp"
#include "handle/hshared.hpp"
#include "animation/animation.hpp"
#include "animation/hitbox_sequence/hitbox_sequence.hpp"
#include "animation/audio_sequence/audio_sequence.hpp"


class animAnimatorSequence {
    RHSHARED<Animation>         anim;
    RHSHARED<hitboxCmdSequence> hitbox_sequence;
    RHSHARED<audioSequence>     audio_sequence;

public:
    void setSkeletalAnimation(const RHSHARED<Animation>& anim) { this->anim = anim; }
    const RHSHARED<Animation>& getSkeletalAnimation() const { return anim; }
    RHSHARED<Animation> getSkeletalAnimation() { return anim; }

    void setHitboxSequence(const RHSHARED<hitboxCmdSequence>& hitbox_sequence) { this->hitbox_sequence = hitbox_sequence; }
    const RHSHARED<hitboxCmdSequence>& getHitboxSequence() const { return hitbox_sequence; }
    RHSHARED<hitboxCmdSequence> getHitboxSequence() { return hitbox_sequence; }

    void setAudioSequence(const RHSHARED<audioSequence>& audio_seq) { this->audio_sequence = audio_seq; }
    const RHSHARED<audioSequence>& getAudioSequence() const { return audio_sequence; }
    RHSHARED<audioSequence> getAudioSequence() { return audio_sequence; }
};