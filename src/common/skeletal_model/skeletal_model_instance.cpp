#include "skeletal_model_instance.hpp"
#include "skeletal_model.hpp"

#include "animation/model_sequence/model_sequence.hpp"


mdlSkeletalModelInstance::~mdlSkeletalModelInstance() {
    if (!prototype) {
        return;
    }
    prototype->destroyInstance(this);
}

void mdlSkeletalModelInstance::applySampleBuffer(animModelSampleBuffer& buf) {
    if (!prototype) {
        assert(false);
        return;
    }
    prototype->applySampleBuffer(this, buf);
}

void mdlSkeletalModelInstance::spawn(scnRenderScene* scn) {
    if (!prototype) {
        assert(false);
        return;
    }
    prototype->spawnInstance(this, scn);
}
void mdlSkeletalModelInstance::despawn(scnRenderScene* scn) {
    if (!prototype) {
        assert(false);
        return;
    }
    prototype->despawnInstance(this, scn);
}