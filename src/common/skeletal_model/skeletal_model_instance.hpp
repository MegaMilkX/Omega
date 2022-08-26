#pragma once

#include <memory>
#include "skeleton/skeleton_instance.hpp"

class sklmSkeletalModelEditable;

struct animModelSampleBuffer;

class sklmSkeletalModelInstance {
    friend sklmSkeletalModelEditable;
public:
    // This struct exists to allow model components to modify an instance during
    // instantiation without friending every possible component type
    // TODO: No longer relevant?
    struct InstanceData {
        HSHARED<sklSkeletonInstance>    skeleton_instance;
        std::vector<char>               instance_data_bytes;
    };
private:
    sklmSkeletalModelEditable* prototype = 0;
    InstanceData instance_data;

public:
    ~sklmSkeletalModelInstance();
    sklSkeletonInstance* getSkeletonInstance() {
        if (!instance_data.skeleton_instance) {
            assert(false);
            return 0;
        }
        return instance_data.skeleton_instance.get();
    }

    void applySampleBuffer(animModelSampleBuffer& buf);
    
    void spawn(scnRenderScene* scn);
    void despawn(scnRenderScene* scn);
};