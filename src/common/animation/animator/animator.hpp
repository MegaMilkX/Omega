#pragma once

#include <unordered_map>
#include <string>
#include "handle/hshared.hpp"
#include "skeleton/skeleton_editable.hpp"
#include "anim_unit.hpp"
#include "animation/animation_sample_buffer.hpp"
#include "animation/animator/anim_unit_single.hpp"
#include "animation/animator/anim_unit_fsm/anim_unit_fsm.hpp"
#include "animation/animator/anim_unit_blend_tree/anim_unit_blend_tree.hpp"

#include "animation/animator/animator_sync_group.hpp"

#include "animation/animator/animator_instance.hpp"

#include "animation/animvm/animvm.hpp"



class AnimatorMaster {
    friend AnimatorInstance;

    RHSHARED<Skeleton> skeleton;
    
    struct SamplerDesc {
        std::string name;
        std::string sync_group;
        RHSHARED<Animation> sequence;
    };
    std::vector<SamplerDesc> samplers;
    std::unordered_map<std::string, int> sampler_names;

    std::unique_ptr<animUnit> rootUnit;

    animvm::vm_program vm_program;
    std::vector<int> signals; // Keeping a list of those to clear them to 0 each update

    animGraphCompileContext compile_context; // Data left after compiling necessary for instantiation

    std::set<HSHARED<AnimatorInstance>> instances;

    void prepareInstance(AnimatorInstance* inst);

public:
    AnimatorMaster() {}

    HSHARED<AnimatorInstance> createInstance();

    int compileExpr(const std::string& source) {
        return animvm::compile(vm_program, source.c_str());
    }

    /// Edit-time
    void setSkeleton(RHSHARED<Skeleton> skl) {
        skeleton = skl;
    }
    Skeleton* getSkeleton() { return skeleton.get(); }

    AnimatorMaster& addSampler(const char* name, const char* sync_group, const RHSHARED<Animation>& sequence) {
        auto it = sampler_names.find(name);
        if (it != sampler_names.end()) {
            assert(false);
            return *this;
        }
        sampler_names[name] = samplers.size();
        samplers.push_back(SamplerDesc{ name, sync_group, sequence });
        return *this;
    }
    int getSamplerId(const char* name) {
        auto it = sampler_names.find(name);
        if (it == sampler_names.end()) {
            assert(false);
            return -1;
        }
        return it->second;
    }

    void setRoot(animUnit* unit) {
        rootUnit.reset(unit);
    }
    animUnit* getRoot() { return rootUnit.get(); }

    int addSignal(const char* name) {
        int addr = vm_program.decl_variable(animvm::type_float, name);
        assert(addr != -1);
        return addr;
    }
    int getSignalId(const char* name) {
        auto var = vm_program.find_variable(name);
        assert(var.addr != -1);
        return var.addr;
    }

    int addFeedbackEvent(const char* name) {
        // TODO: feedback events should be stored on the host side too
        auto id = vm_program.decl_host_event(name, -1);
        assert(id != -1);
        return id;
    }
    int getFeedbackEventId(const char* name) {
        auto event = vm_program.find_host_event(name);
        assert(event.id != -1);
        return event.id;
    }

    int addParam(const char* name) {
        int addr = vm_program.decl_variable(animvm::type_float, name);
        assert(addr != -1);
        return addr;
    }
    int getParamId(const char* name) {
        auto var = vm_program.find_variable(name);
        assert(var.addr != -1);
        return var.addr;
    }

    bool compile() {
        assert(skeleton);
        assert(rootUnit);
        if (!skeleton || !rootUnit) {
            LOG_ERR("AnimatorMaster missing skeleton or rootUnit");
            return false;
        }

        vm_program.decl_variable(animvm::type_bool, "state_complete");

        // Init animator tree
        compile_context = animGraphCompileContext();
        rootUnit->compile(&compile_context, this, skeleton.get());
        return true;
    }
};