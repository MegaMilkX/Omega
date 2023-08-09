#pragma once

#include <memory>
#include "skeleton/skeleton_instance.hpp"

class mdlSkeletalModelMaster;

struct animModelSampleBuffer;

class mdlSkeletalModelInstance {
    friend mdlSkeletalModelMaster;
public:
    // This struct exists to allow model components to modify an instance during
    // instantiation without friending every possible component type
    // TODO: No longer relevant?
    struct InstanceData {
        HSHARED<SkeletonPose>    skeleton_instance;
        std::vector<char>               instance_data_bytes;
    };
private:
    mdlSkeletalModelMaster* prototype = 0;
    InstanceData instance_data;

public:
    ~mdlSkeletalModelInstance();
    SkeletonPose* getSkeletonInstance() {
        if (!instance_data.skeleton_instance) {
            assert(false);
            return 0;
        }
        return instance_data.skeleton_instance.get();
    }
    Skeleton* getSkeletonMaster() {
        auto inst = getSkeletonInstance();
        if (!inst) {
            return 0;
        }
        return inst->getSkeletonMaster();
    }

    void applySampleBuffer(animModelSampleBuffer& buf);
    
    void spawn(scnRenderScene* scn);
    void despawn(scnRenderScene* scn);
};