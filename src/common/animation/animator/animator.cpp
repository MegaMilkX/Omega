#include "animator.hpp"

#include "animation/animator/anim_unit_single.hpp"
#include "animation/animator/anim_unit_fsm/anim_unit_fsm.hpp"
#include "animation/animator/anim_unit_blend_tree/anim_unit_blend_tree.hpp"


AnimMachine& AnimMachine::addSampler(const char* name, const char* sync_group, const ResourceRef<Animation>& sequence) {
    auto it = sampler_names.find(name);
    if (it != sampler_names.end()) {
        assert(false);
        return *this;
    }
    sampler_names[name] = samplers.size();
    samplers.push_back(SamplerDesc{ name, sync_group, sequence });
    return *this;
}
int AnimMachine::getSamplerId(const char* name) {
    auto it = sampler_names.find(name);
    if (it == sampler_names.end()) {
        assert(false);
        return -1;
    }
    return it->second;
}

int AnimMachine::addSignal(const char* name) {
    int addr = vm_program.decl_variable(animvm::type_float, name);
    assert(addr != -1);
    return addr;
}
int AnimMachine::getSignalId(const char* name) {
    auto var = vm_program.find_variable(name);
    assert(var.addr != -1);
    return var.addr;
}


int AnimMachine::addFeedbackEvent(const char* name) {
    // TODO: feedback events should be stored on the host side too
    auto id = vm_program.decl_host_event(name, -1);
    assert(id != -1);
    return id;
}
int AnimMachine::getFeedbackEventId(const char* name) {
    auto event = vm_program.find_host_event(name);
    assert(event.id != -1);
    return event.id;
}

int AnimMachine::addParam(const char* name) {
    int addr = vm_program.decl_variable(animvm::type_float, name);
    assert(addr != -1);
    return addr;
}
int AnimMachine::getParamId(const char* name) {
    auto var = vm_program.find_variable(name);
    assert(var.addr != -1);
    return var.addr;
}

bool AnimMachine::compile() {
    assert(skeleton);
    assert(rootUnit);
    if (!skeleton || !rootUnit) {
        LOG_ERR("AnimMachine missing skeleton or rootUnit");
        return false;
    }

    vm_program.decl_variable(animvm::type_bool, "state_complete");

    // Init animator tree
    compile_context = animGraphCompileContext();
    rootUnit->compile(&compile_context, this, skeleton.get());
    return true;
}

