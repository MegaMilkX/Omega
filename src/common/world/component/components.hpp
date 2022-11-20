#pragma once

#include "actor_component.hpp"

#include "math/gfxm.hpp"

#include "skeleton/skeleton_editable.hpp"
#include "skeleton/skeleton_instance.hpp"

#include "animation/animator/animator.hpp"
#include "animation/animator/animator_instance.hpp"

class AnimatorComponent : public ActorComponent {
    RHSHARED<AnimatorMaster> animator;
    RHSHARED<animAnimatorSequence> seq_idle;
    RHSHARED<animAnimatorSequence> seq_run2;
    RHSHARED<animAnimatorSequence> seq_open_door_front;
    RHSHARED<animAnimatorSequence> seq_open_door_back;

    RHSHARED<audioSequence> audio_seq;

    HSHARED<animAnimatorInstance> anim_inst;
public:
    AnimatorComponent() {
        // Temporary
        audio_seq.reset_acquire();
        audio_seq->length = 40.0f;
        audio_seq->fps = 60.0f;
        audio_seq->insert(0, resGet<AudioClip>("audio/sfx/gravel1.ogg"));
        audio_seq->insert(20, resGet<AudioClip>("audio/sfx/gravel2.ogg"));


        seq_idle.reset_acquire();
        seq_run2.reset_acquire();
        seq_open_door_front.reset_acquire();
        seq_open_door_back.reset_acquire();
        seq_idle->setSkeletalAnimation(resGet<Animation>("models/chara_24/Idle.animation"));
        seq_run2->setSkeletalAnimation(resGet<Animation>("models/chara_24/Run2.animation"));
        seq_run2->setAudioSequence(audio_seq);
        seq_open_door_front->setSkeletalAnimation(resGet<Animation>("models/chara_24_anim_door/Action_OpenDoor.animation"));
        seq_open_door_back->setSkeletalAnimation(resGet<Animation>("models/chara_24/Action_DoorOpenBack.animation"));

        animator.reset_acquire();
        animator->setSkeleton(resGet<sklSkeletonMaster>("models/chara_24/chara_24.skeleton"));
        // Setup parameters signals and events
        animator->addParam("velocity");
        animator->addSignal("sig_door_open");
        animator->addSignal("sig_door_open_back");
        animator->addFeedbackEvent("fevt_door_open_end");
        // Add samplers
        animator
            ->addSampler("idle", "Default", seq_idle)
            .addSampler("run", "Locomotion", seq_run2)
            .addSampler("open_door_front", "Interact", seq_open_door_front)
            .addSampler("open_door_back", "Interact", seq_open_door_back);

        // Setup the tree
        auto fsm = animator->setRoot<animUnitFsm>();
        auto st_idle = fsm->addState("Idle");
        auto st_loco = fsm->addState("Locomotion");
        auto st_door_open_front = fsm->addState("DoorOpenFront");
        auto st_door_open_back = fsm->addState("DoorOpenBack");
        st_idle->setUnit<animUnitSingle>()->setSampler("idle");
        st_loco->setUnit<animUnitSingle>()->setSampler("run");
        st_door_open_front->setUnit<animUnitSingle>()->setSampler("open_door_front");
        st_door_open_front->onExit(call_feedback_event_(animator.get(), "fevt_door_open_end"));
        st_door_open_back->setUnit<animUnitSingle>()->setSampler("open_door_back");
        st_door_open_back->onExit(call_feedback_event_(animator.get(), "fevt_door_open_end"));
        fsm->addTransition("Idle", "Locomotion", param_(animator.get(), "velocity") > FLT_EPSILON, 0.15f);
        fsm->addTransition("Locomotion", "Idle", param_(animator.get(), "velocity") <= FLT_EPSILON, 0.15f);
        fsm->addTransitionAnySource("DoorOpenFront", signal_(animator.get(), "sig_door_open"), 0.15f);
        fsm->addTransitionAnySource("DoorOpenBack", signal_(animator.get(), "sig_door_open_back"), 0.15f);
        fsm->addTransition("DoorOpenFront", "Idle", state_complete_(), 0.15f);
        fsm->addTransition("DoorOpenBack", "Idle", state_complete_(), 0.15f);
        animator->compile();

        anim_inst = animator->createInstance();
    }

    animAnimatorInstance* getAnimatorInstance() { return anim_inst.get(); }
    AnimatorMaster* getAnimatorMaster() { return animator.get(); }
    sklSkeletonMaster* getSkeletonMaster() { return animator->getSkeleton(); }
};