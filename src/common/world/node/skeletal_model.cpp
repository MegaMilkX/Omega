#include "skeletal_model.hpp"


SkeletalModelNode2::SkeletalModelNode2() {
    instance.attachTo(getTransformHandle());
    SceneProxy::setTransformNode(getTransformHandle());
}

void SkeletalModelNode2::setModel(const ResourceRef<m3dModel>& mdl) {
    model = mdl;
    requestRebuild();
}

ResourceRef<m3dModel> SkeletalModelNode2::getModel() const {
    return instance.getModelRef();
}

void SkeletalModelNode2::setLayer(int i) {
    instance.setLayer(i);
}
int SkeletalModelNode2::getLayer() const {
    return instance.getLayer();
}

HTransform SkeletalModelNode2::getBoneProxy(const std::string& name) {
    if (!instance.getSkeletonInstance()) {
        return HTransform();
    }
    int i = instance.getSkeletonInstance()->findBoneIndex(name.c_str());
    if (i < 0) {
        return HTransform();
    }
    return getBoneProxy(i);
}

HTransform SkeletalModelNode2::getBoneProxy(int idx) {
    return instance.getSkeletonInstance()->getBoneNode(idx);
}

void SkeletalModelNode2::enableTechnique(const std::string path, bool value) {
    instance.enableTechnique(path, value);
}
void SkeletalModelNode2::setRenderParam(const char* param_name, GPU_TYPE type, const void* pvalue) {
    instance.setParam(param_name, type, pvalue);
}

void SkeletalModelNode2::onBuild() {

}

const NodeSlotDescArray& SkeletalModelNode2::getSlots() {
    static NodeSlotDescArray slots = {
        NodeSlotDesc{ rtti::type_get<HSHARED<SkeletonInstance>>(), LINK_READ | LINK_WRITE, eSlotUpstream }
    };
    return slots;
}
void SkeletalModelNode2::onLinkRead(int slot, const rtti::varying& in) {
    if(slot == 0) {
        external_skeleton = *in.get<HSHARED<SkeletonInstance>>();
    }
}

void SkeletalModelNode2::onReady() {
    instance.init(model, external_skeleton);
}

void SkeletalModelNode2::onSpawnActorNode(WorldSystemRegistry& reg) {
    if (auto scn = reg.getSystem<SceneSystem>()) {
        scene_sys = scn;
        scn->addProxy(this);
    }
}

void SkeletalModelNode2::onDespawnActorNode(WorldSystemRegistry& reg) {
    if (auto scn = reg.getSystem<SceneSystem>()) {
        scn->removeProxy(this);
        scene_sys = nullptr;
    }
}

void SkeletalModelNode2::updateBounds() {
    const m3dModel* model = instance.getModel();
    if (!model) {
        assert(false);
        return;
    }
    const gfxm::mat4& transform = getTransformHandle()->getWorldTransform();

    setBoundingSphere(
        model->bounding_radius,
        transform * gfxm::vec4(model->bounding_sphere_origin, 1.f)
    ); // TODO: scaling

    gfxm::aabb box = model->aabb;
    const gfxm::vec3 points[] = {
        transform * gfxm::vec4(box.from, 1.f),
        transform * gfxm::vec4(box.to.x, box.from.y, box.from.z, 1.f),
        transform * gfxm::vec4(box.to.x, box.from.y, box.to.z, 1.f),
        transform * gfxm::vec4(box.from.x, box.from.y, box.to.z, 1.f),
        transform * gfxm::vec4(box.from.x, box.to.y, box.from.z, 1.f),
        transform * gfxm::vec4(box.to.x, box.to.y, box.from.z, 1.f),
        transform * gfxm::vec4(box.to, 1.f),
        transform * gfxm::vec4(box.from.x, box.to.y, box.to.z, 1.f)
    };
    box.from = points[0];
    box.to = points[0];
    for (int i = 1; i < sizeof(points) / sizeof(points[0]); ++i) {
        gfxm::expand_aabb(box, points[i]);
    }
    setBoundingBox(box);
}
void SkeletalModelNode2::submit(gpuRenderBucket* bucket) {
    instance.submit(bucket);
}

void SkeletalModelNode2::onSnapshot() {
    // setModel already requests rebuild, nothing to do here?
}

