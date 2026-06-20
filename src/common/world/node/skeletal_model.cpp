#include "skeletal_model.hpp"


SkeletalModelNode2::SkeletalModelNode2() {
    instance.attachTo(getTransformHandle());
}

void SkeletalModelNode2::setModel(const ResourceRef<m3dModel>& mdl) {
    model = mdl;
}

ResourceRef<m3dModel> SkeletalModelNode2::getModel() const {
    return instance.getModelRef();
}

void SkeletalModelNode2::onSpawnActorNode(WorldSystemRegistry& reg) {
    instance.init(model, external_skeleton);

    if (auto scn = reg.getSystem<SceneSystem>()) {
        scn->addProxy(this);
    }
}

void SkeletalModelNode2::onDespawnActorNode(WorldSystemRegistry& reg) {
    if (auto scn = reg.getSystem<SceneSystem>()) {
        scn->removeProxy(this);
    }
}

const NodeSlotDescArray& SkeletalModelNode2::getSlots() {
    static NodeSlotDescArray slots = {
        NodeSlotDesc{ type_get<HSHARED<SkeletonInstance>>(), LINK_READ | LINK_WRITE, eSlotUpstream }
    };
    return slots;
}
void SkeletalModelNode2::onLinkRead(int slot, const varying& in) {
    if(slot == 0) {
        external_skeleton = *in.get<HSHARED<SkeletonInstance>>();
        markDirty();
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

