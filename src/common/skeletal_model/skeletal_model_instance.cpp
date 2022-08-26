#include "skeletal_model_instance.hpp"
#include "skeletal_model.hpp"

#include "animation/model_sequence/model_sequence.hpp"


sklmSkeletalModelInstance::~sklmSkeletalModelInstance() {
    if (!prototype) {
        return;
    }
    prototype->destroyInstance(this);
}

void sklmSkeletalModelInstance::applySampleBuffer(animModelSampleBuffer& buf) {
    if (!prototype) {
        assert(false);
        return;
    }
    prototype->applySampleBuffer(this, buf);
}

void sklmSkeletalModelInstance::spawn(scnRenderScene* scn) {
    if (!prototype) {
        assert(false);
        return;
    }
    prototype->spawnInstance(this, scn);
}
void sklmSkeletalModelInstance::despawn(scnRenderScene* scn) {
    if (!prototype) {
        assert(false);
        return;
    }
    prototype->despawnInstance(this, scn);
}