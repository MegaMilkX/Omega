#pragma once

#include "scn_render_object.hpp"

#include "resource/resource.hpp"

#include "platform/platform.hpp"

#include "gpu/renderable/decal.hpp"


class scnRenderScene;
class scnDecal : public scnRenderObject {

    friend scnRenderScene;

    gpuDecalRenderable* renderable = nullptr;
    RHSHARED<gpuMaterial> material;
    gfxm::vec3 extents = gfxm::vec3(1.0f, 1.0f, 1.0f);

    void onAdded() override {
        for (int i = 0; i < renderableCount(); ++i) {
            if (renderables[i]->getMaterial() && renderables[i]->getMeshDesc()) {
                renderables[i]->compile();
            }
        }
    }
    void onRemoved() override {

    }

public:
    TYPE_ENABLE();
    scnDecal()
    : scnRenderObject(false) {
        renderable = new gpuDecalRenderable();
        transform_block = renderable->getTransformBlock();
        //ubuf_model = renderable->getModelUniformBuffer();

        renderable->dbg_name = "scnDecal";
        addRenderable(renderable);
        renderable->setColor(gfxm::vec4(1, 1, 1, 1));
        extents = gfxm::vec3(1, 1, 1);
        renderable->setExtents(extents);
    }

    void setMaterial(RHSHARED<gpuMaterial> material_) {
        this->material = material_;
        renderable->setMaterial(material_.get());
    }
    RHSHARED<gpuMaterial> getMaterial() const {
        return this->material;
    }

    void setBoxSize(float x, float y, float z) {
        setBoxSize(gfxm::vec3(x, y, z));
    }
    void setBoxSize(const gfxm::vec3& boxSize) {
        extents = boxSize;
        renderable->setExtents(extents);
    }
    const gfxm::vec3& getBoxSize() const { return extents; }
    void setColor(const gfxm::vec4& rgba) {
        renderable->setColor(rgba);
    }

    static void reflect() {
        type_register<scnDecal>("scnDecal")
            .parent<scnRenderObject>();
    }
};