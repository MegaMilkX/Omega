#pragma once

#include <unordered_map>
#include <string>
#include "handle/hshared.hpp"
#include "resource_manager/loadable.hpp"
#include "skeleton/skeleton_editable.hpp"
#include "anim_unit.hpp"
#include "animation/animation_sample_buffer.hpp"

#include "animation/animator/animator_sync_group.hpp"

#include "animation/animvm/animvm.hpp"



class AnimMachine : public ILoadable {
    friend AnimMachineInstance;

    ResourceRef<Skeleton> skeleton;
    
    struct SamplerDesc {
        std::string name;
        std::string sync_group;
        ResourceRef<Animation> sequence;
    };
    std::vector<SamplerDesc> samplers;
    std::unordered_map<std::string, int> sampler_names;

    std::unique_ptr<animUnit> rootUnit;

    animvm::vm_program vm_program;
    std::vector<int> signals; // Keeping a list of those to clear them to 0 each update

    animGraphCompileContext compile_context; // Data left after compiling necessary for instantiation

    std::set<HSHARED<AnimMachineInstance>> instances;

    //void prepareInstance(AnimMachineInstance* inst);

public:
    AnimMachine() {}

    //HSHARED<AnimMachineInstance> createInstance();

    int compileExpr(const std::string& source) {
        return animvm::compile(vm_program, source.c_str());
    }

    /// Edit-time
    void setSkeleton(ResourceRef<Skeleton> skl) {
        skeleton = skl;
    }
    Skeleton* getSkeleton() { return skeleton.get(); }

    AnimMachine& addSampler(const char* name, const char* sync_group, const ResourceRef<Animation>& sequence);
    int getSamplerId(const char* name);

    void setRoot(animUnit* unit) {
        rootUnit.reset(unit);
    }
    animUnit* getRoot() { return rootUnit.get(); }

    int addSignal(const char* name);
    int getSignalId(const char* name);

    int addFeedbackEvent(const char* name);
    int getFeedbackEventId(const char* name);

    int addParam(const char* name);
    int getParamId(const char* name);

    bool compile();

    DEFINE_EXTENSIONS(e_amp);
    bool load(byte_reader& in) override {
        // TODO:
        assert(false);
        return false;
    }
};

