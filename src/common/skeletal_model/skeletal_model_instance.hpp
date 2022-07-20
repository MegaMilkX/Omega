#pragma once

#include <memory>
#include "skeleton/skeleton_instance.hpp"

class sklmSkeletalModelEditable;


class sklmSkeletalModelInstance {
    friend sklmSkeletalModelEditable;
public:
    // This struct exists to allow model components to modify an instance during
    // instantiation without friending every possible component type
    struct InstanceData {
        HSHARED<sklSkeletonInstance>                    skeleton_instance;
        std::vector<std::unique_ptr<scnRenderObject>>   render_objects;
    };
private:
    InstanceData instance_data;

public:
    sklSkeletonInstance* getSkeletonInstance() {
        if (!instance_data.skeleton_instance) {
            assert(false);
            return 0;
        }
        return instance_data.skeleton_instance.get();
    }

    void onSpawn(scnRenderScene* scn) {
        assert(!instance_data.render_objects.empty());

        instance_data.skeleton_instance->onSpawn(scn);
        for (int i = 0; i < instance_data.render_objects.size(); ++i) {
            scn->addRenderObject(instance_data.render_objects[i].get());
        }
    }
    void onDespawn(scnRenderScene* scn) {
        assert(!instance_data.render_objects.empty());

        for (int i = 0; i < instance_data.render_objects.size(); ++i) {
            scn->removeRenderObject(instance_data.render_objects[i].get());
        }
        instance_data.skeleton_instance->onDespawn(scn);
    }
};