#include "skeleton_editable.hpp"

#include <assert.h>
#include <queue>
#include <vector>

#include "skeleton_instance.hpp"

#include "log/log.hpp"

#include "util/static_block.hpp"
#include "reflection/reflection.hpp"
#include "resource/resource.hpp"

#include "resource_manager/byte_writer/file_writer.hpp"


void Skeleton::rebuildBoneArray() {
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

    fingerprint = computeFingerprint();
}

uint64_t Skeleton::computeFingerprint() const {
    constexpr uint64_t FNV_OFFSET = 14695981039346656037ULL;
    constexpr uint64_t FNV_PRIME  = 1099511628211ULL;

    uint64_t hash = FNV_OFFSET;

    for (int i = 0; i < boneCount(); ++i) {
        auto bone = getBone(i);
        const char* name = bone->getName().c_str();
        while (*name) {
            hash ^= (uint64_t)(unsigned char)*name++;
            hash *= FNV_PRIME;
        }
        // include a separator so "abcd" + "ef" != "ab" + "cdef"
        hash ^= 0xFF;
        hash *= FNV_PRIME;
    }

    return hash;
}


void Skeleton::clear() {
    root.reset(new sklBone(this, 0, "Root"));
    rebuildBoneArray();
}

void Skeleton::setBoneName(int idx, const char* name) {
    sklBone* bone = getBone(idx);
    assert(bone);
    if (!bone) {
        return;
    }
    
    auto it = name_to_index.find(bone->getName());
    assert(it != name_to_index.end());
    name_to_index.erase(it);

    name_to_index.insert(std::make_pair(std::string(name), idx));
    bone->setName(name);
}
void Skeleton::setRootName(const char* name) {
    setBoneName(0, name);
}

const int* Skeleton::getParentArrayPtr() const {
    return parent_array.data();
}
const std::vector<sklBone*>&    Skeleton::getBoneArray() const {
    return bone_array;
}
std::vector<gfxm::mat4>         Skeleton::makeLocalTransformArray() const {
    std::vector<gfxm::mat4> arr;
    arr.resize(bone_array.size());
    for (int i = 0; i < bone_array.size(); ++i) {
        arr[i] = bone_array[i]->getLocalTransform();
    }
    return arr;
}
std::vector<gfxm::mat4>         Skeleton::makeWorldTransformArray() const {
    std::vector<gfxm::mat4> arr;
    arr.resize(bone_array.size());
    for (int i = 0; i < bone_array.size(); ++i) {
        arr[i] = bone_array[i]->getWorldTransform();
    }
    return arr;
}

HSHARED<SkeletonInstance> Skeleton::createInstance() {
    HSHARED<SkeletonInstance> hinstance(HANDLE_MGR<SkeletonInstance>::acquire());
    instances.insert(hinstance);
    
    hinstance->prototype = this;

    auto lcl_transforms = makeLocalTransformArray();
    auto world_transforms = makeWorldTransformArray();

    hinstance->bone_nodes.resize(boneCount());
    for (int i = 0; i < boneCount(); ++i) {
        hinstance->bone_nodes[i].acquire();
        hinstance->bone_nodes[i]->setTranslation(bone_array[i]->getLclTranslation());
        hinstance->bone_nodes[i]->setRotation(bone_array[i]->getLclRotation());
        hinstance->bone_nodes[i]->setScale(bone_array[i]->getLclScale());
    }
    for (int i = 0; i < boneCount(); ++i) {
        int parent_idx = parent_array[i];
        if (parent_idx != -1) {
            transformNodeAttach(hinstance->bone_nodes[parent_idx], hinstance->bone_nodes[i]);
        }
    }

    return hinstance;
}
void Skeleton::destroyInstance(HSHARED<SkeletonInstance> inst) {
    if (!inst) {
        assert(false);
        return;
    }
    if (inst->getSkeletonMaster() != this) {
        assert(false);
        return;
    }
    instances.erase(inst);
    inst.reset(0);
}


bool Skeleton::merge(Skeleton& other) {
    // TODO
    assert(false);

    return true;
}


void Skeleton::dbgLog() {
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

bool Skeleton::load_json(byte_reader& in) {
    auto view = in.try_slurp();
    if (!view) {
        return false;
    }
    std::string str_json(view.data, view.data + view.size);
    nlohmann::json j = nlohmann::json::parse(str_json);
    if (!j.is_object()) {
        return false;
    }

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

    getRoot()->setName(bones_deserialized[0].name.c_str());
    getRoot()->setTranslation(bones_deserialized[0].translation);
    getRoot()->setRotation(bones_deserialized[0].rotation);
    getRoot()->setScale(bones_deserialized[0].scale);
    for (int i = 1; i < bones_deserialized.size(); ++i) {
        auto& bd = bones_deserialized[i];
        sklBone* parent = getBone(bd.parent_index);
        sklBone* bone = parent->createChild(bd.name.c_str());
        bone->setTranslation(bd.translation);
        bone->setRotation(bd.rotation);
        bone->setScale(bd.scale);
    }
    return true;
}

constexpr uint32_t SKL_TAG = 'S' | ('K' << 8) | ('L' << 16) | ('\0' << 24);
constexpr uint32_t SKL_VERSION = 1;

#pragma pack(push, 1)
struct SKL_HEAD {
    uint32_t tag = 0;
    uint32_t version = 0;

    uint64_t fingerprint = 0;

    uint32_t offs_bones = 0;
};
#pragma pack(pop)

bool Skeleton::load(byte_reader& in) {
    size_t rewind_point = in.tell();
    SKL_HEAD head = { 0 };
    in.read<SKL_HEAD>(&head);

    if (head.tag != SKL_TAG) {
        in.seek(rewind_point, byte_reader::seek_set);
        return load_json(in);
    }

    LOG_DBG("SKL version " << head.version);
    LOG_DBG("fingerprint 0x" << std::hex << head.fingerprint);

    in.seek(head.offs_bones, byte_reader::seek_set);

    uint32_t bone_count = 0;
    in.read<uint32_t>(&bone_count);
    if (bone_count < 1) {
        assert(false);
        LOG_ERR("Skeleton::load: skeleton must have at least one bone");
        return false;
    }
    LOG_DBG("bone count: " << bone_count);
    bone_array.resize(bone_count);
    for (int i = 0; i < bone_count; ++i) {
        sklBone* bone = new sklBone(this, nullptr, "");
        bone_array[i] = bone;
    }

    for (int i = 0; i < bone_count; ++i) {
        uint32_t index = i;
        int32_t parent_index = -1;
        std::string name;
        gfxm::vec3 trans;
        gfxm::quat rot;
        gfxm::vec3 scl;
        in.read<int32_t>(&parent_index);
        in.read_string(&name);
        in.read<gfxm::vec3>(&trans);
        in.read<gfxm::quat>(&rot);
        in.read<gfxm::vec3>(&scl);

        if (i == 0 && parent_index != -1) {
            assert(false);
            LOG_ERR("Skeleton::load: First bone must be root");
            return false;
        }

        sklBone* bone = bone_array[i];
        bone->index = i;
        bone->name = name;
        if (parent_index >= 0) {
            bone->parent = bone_array[parent_index];
            bone->parent->children.push_back(bone);
        }
        bone->setTranslation(trans);
        bone->setRotation(rot);
        bone->setScale(scl);
    }
    root.reset(bone_array[0]);

    rebuildBoneArray();
    return true;
}

void Skeleton::write(const std::string& path) const {
	FILE* f = fopen(path.c_str(), "wb");
    file_writer out(f);
    write(out);
	fclose(f);
}
void Skeleton::write(byte_writer& out) const {
    SKL_HEAD head;
    head.tag = SKL_TAG;
    head.version = SKL_VERSION;
    head.fingerprint = computeFingerprint();
    LOG_DBG("SKL fingerprint on write 0x" << std::hex << head.fingerprint);

    out.write<SKL_HEAD>(head);

    head.offs_bones = out.tell();
    uint32_t bone_count = boneCount();
    out.write<uint32_t>(bone_count);
    for (int i = 0; i < bone_array.size(); ++i) {
        auto bone = bone_array[i];
        auto parent = bone->getParent();

        int32_t parent_index = parent ? parent->getIndex() : -1;
        out.write<int32_t>(parent_index);
        out.write_string(bone->getName());

        out.write<gfxm::vec3>(bone->getLclTranslation());
        out.write<gfxm::quat>(bone->getLclRotation());
        out.write<gfxm::vec3>(bone->getLclScale());
    }

    size_t rewind = out.tell();
    out.seek(0, byte_writer::seek_set);
    out.write<SKL_HEAD>(head);
    out.seek(rewind, byte_writer::seek_set); // seek back to the "end" in case we're being directly embedded
}

void Skeleton::toJson(nlohmann::json& j) {
    auto& bone_array = getBoneArray();

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
}


