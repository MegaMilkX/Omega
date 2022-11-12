#include "skeleton_editable.hpp"

#include <assert.h>
#include <queue>
#include <vector>

#include "skeleton_prototype.hpp"
#include "skeleton_instance.hpp"

#include "log/log.hpp"

#include "util/static_block.hpp"
#include "reflection/reflection.hpp"
#include "resource/resource.hpp"
STATIC_BLOCK{
    sklSkeletonMaster::reflect();
    
    resAddCache<sklSkeletonMaster>(new resCacheDefault<sklSkeletonMaster>);
}

void sklSkeletonMaster::reflect() {
    type_register<sklSkeletonMaster>("sklSkeletonMaster")
        .custom_serialize_json([](nlohmann::json& j, void* object) {
            auto o = (sklSkeletonMaster*)object;
            
            auto& bone_array = o->getBoneArray();
            
            nlohmann::json& j_bones = j["bones"];
            for (int i = 0; i < bone_array.size(); ++i) {
                auto bone = bone_array[i];
                auto parent = bone->getParent();
                nlohmann::json& j_bone = j_bones[i];
                j_bone["name"] = bone->getName();
                j_bone["index"] = bone->getIndex();                
                if (parent) {
                    j_bone["parent"] = parent->getName();
                    j_bone["parent_index"] = parent->getIndex();
                } else {
                    j_bone["parent"] = nullptr;
                    j_bone["parent_index"] = -1;
                }
                j_bone["translation"] = { bone->getLclTranslation().x, bone->getLclTranslation().y, bone->getLclTranslation().z };
                j_bone["rotation"] = { bone->getLclRotation().x, bone->getLclRotation().y, bone->getLclRotation().z, bone->getLclRotation().w };
                j_bone["scale"] = { bone->getLclScale().x, bone->getLclScale().y, bone->getLclScale().z };
            }
        })
        .custom_deserialize_json([](nlohmann::json& j, void* object) {
            auto o = (sklSkeletonMaster*)object;

            std::vector<BoneDeserialized> bones_deserialized;

            nlohmann::json j_bones = j["bones"];
            assert(j_bones.is_array());
            for (int i = 0; i < j_bones.size(); ++i) {
                BoneDeserialized bd;
                nlohmann::json j_bone = j_bones[i];
                assert(j_bone.is_object());
                auto& j_name = j_bone["name"];
                auto& j_index = j_bone["index"];
                auto& j_parent_index = j_bone["parent_index"];
                auto& j_parent = j_bone["parent"];
                auto& j_translation = j_bone["translation"];
                auto& j_rotation = j_bone["rotation"];
                auto& j_scale = j_bone["scale"];
                assert(j_translation.is_array() && j_translation.size() == 3);
                assert(j_rotation.is_array() && j_rotation.size() == 4);
                assert(j_scale.is_array() && j_scale.size() == 3);

                bd.translation = gfxm::vec3(j_translation[0].get<float>(), j_translation[1].get<float>(), j_translation[2].get<float>());
                bd.rotation = gfxm::quat(j_rotation[0].get<float>(), j_rotation[1].get<float>(), j_rotation[2].get<float>(), j_rotation[3].get<float>());
                bd.scale =gfxm::vec3(j_scale[0].get<float>(), j_scale[1].get<float>(), j_scale[2].get<float>());
                bd.name = j_name.get<std::string>();
                bd.index = j_index.get<int>();
                bd.parent_index = j_parent_index.get<int>();
                if (!j_parent.is_null()) { bd.parent_name = j_parent.get<std::string>(); }
                
                bones_deserialized.push_back(bd);
            }

            std::sort(bones_deserialized.begin(), bones_deserialized.end(), [](const BoneDeserialized& a, const BoneDeserialized& b)->bool {
                return a.index < b.index;
            });

            o->getRoot()->setName(bones_deserialized[0].name.c_str());
            o->getRoot()->setTranslation(bones_deserialized[0].translation);
            o->getRoot()->setRotation(bones_deserialized[0].rotation);
            o->getRoot()->setScale(bones_deserialized[0].scale);
            for (int i = 1; i < bones_deserialized.size(); ++i) {
                auto& bd = bones_deserialized[i];
                sklBone* parent = o->getBone(bd.parent_index);
                sklBone* bone = parent->createChild(bd.name.c_str());
                bone->setTranslation(bd.translation);
                bone->setRotation(bd.rotation);
                bone->setScale(bd.scale);
            }
        });
}


void sklSkeletonMaster::rebuildBoneArray() {
    bone_array.clear();
    parent_array.clear();
    name_to_index.clear();

    sklBone* bone = getRoot();
    std::queue<sklBone*> bone_q;
    while (bone) {
        for (int i = 0; i < bone->childCount(); ++i) {
            auto child = bone->getChild(i);
            bone_q.push(child);
        }

        bone_array.push_back(bone);

        if (bone_q.empty()) {
            bone = 0;
        } else {
            bone = bone_q.front();
            bone_q.pop();
        }
    }

    parent_array.resize(bone_array.size());
    for (int i = 0; i < bone_array.size(); ++i) {
        bone_array[i]->index = i;
        parent_array[i] = (bone_array[i]->parent ? bone_array[i]->parent->index : -1);
        name_to_index[bone_array[i]->getName()] = i;
    }
}


void sklSkeletonMaster::clear() {
    root.reset(new sklBone(this, 0, "Root"));
    rebuildBoneArray();
}

const int* sklSkeletonMaster::getParentArrayPtr() const {
    return parent_array.data();
}
const std::vector<sklBone*>&    sklSkeletonMaster::getBoneArray() const {
    return bone_array;
}
std::vector<gfxm::mat4>         sklSkeletonMaster::makeLocalTransformArray() const {
    std::vector<gfxm::mat4> arr;
    arr.resize(bone_array.size());
    for (int i = 0; i < bone_array.size(); ++i) {
        arr[i] = bone_array[i]->getLocalTransform();
    }
    return arr;
}
std::vector<gfxm::mat4>         sklSkeletonMaster::makeWorldTransformArray() const {
    std::vector<gfxm::mat4> arr;
    arr.resize(bone_array.size());
    for (int i = 0; i < bone_array.size(); ++i) {
        arr[i] = bone_array[i]->getWorldTransform();
    }
    return arr;
}

HSHARED<sklSkeletonInstance> sklSkeletonMaster::createInstance() {
    HSHARED<sklSkeletonInstance> hs(HANDLE_MGR<sklSkeletonInstance>::acquire());
    instances.insert(hs);
    
    hs->prototype = this;
    hs->local_transforms = new gfxm::mat4[boneCount()];
    hs->world_transforms = new gfxm::mat4[boneCount()];

    auto lcl_transforms = makeLocalTransformArray();
    auto world_transforms = makeWorldTransformArray();
    memcpy(hs->local_transforms, lcl_transforms.data(), boneCount() * sizeof(gfxm::mat4));
    memcpy(hs->world_transforms, world_transforms.data(), boneCount() * sizeof(gfxm::mat4));

    hs->scn_skel.reset(new scnSkeleton);
    hs->scn_skel->bone_count = boneCount();
    hs->scn_skel->local_transforms = hs->local_transforms;
    hs->scn_skel->world_transforms = hs->world_transforms;
    hs->scn_skel->parents = getParentArrayPtr();

    return hs;
}


bool sklSkeletonMaster::merge(sklSkeletonMaster& other) {
    // TODO
    assert(false);

    return true;
}

bool sklSkeletonMaster::makePrototype(sklSkeletonPrototype* proto) {
    if (proto->parents.size() != 0 || proto->name_to_index.size() != 0) {
        assert(false);
        LOG_ERR("Skeleton prototype is already initialized");
        return false;
    }

    proto->name_to_index = name_to_index;
    proto->parents = std::vector<int>(getParentArrayPtr(), getParentArrayPtr() + boneCount());
    proto->default_pose = makeLocalTransformArray();

    return true;
}


void sklSkeletonMaster::addDependant(sklSkeletonDependant* dep) {
    dependants.insert(dep);
}
void sklSkeletonMaster::removeDependant(sklSkeletonDependant* dep) {
    dependants.erase(dep);
}


void sklSkeletonMaster::dbgLog() {
    sklBone* bone = getRoot();
    std::queue<sklBone*> bone_q;
    while (bone) {
        for (int i = 0; i < bone->childCount(); ++i) {
            auto child = bone->getChild(i);
            bone_q.push(child);
        }

        gfxm::vec3 t = bone->getLclTranslation();
        gfxm::quat r = bone->getLclRotation();
        gfxm::vec3 s = bone->getLclScale();
        LOG_DBG(bone->getName());

        if (bone_q.empty()) {
            bone = 0;
        } else {
            bone = bone_q.front();
            bone_q.pop();
        }
    }
}


