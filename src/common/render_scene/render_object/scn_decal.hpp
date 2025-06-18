#pragma once

#include "scn_render_object.hpp"

#include "resource/resource.hpp"

#include "platform/platform.hpp"


class scnRenderScene;
class scnDecal : public scnRenderObject {

    friend scnRenderScene;

    RHSHARED<gpuMaterial> material;
    std::unique_ptr<gpuDecalUniformBuffer> ubufDecal;
    gfxm::vec3 boxSize = gfxm::vec3(1.0f, 1.0f, 1.0f);
    gpuBuffer vertexBuffer;
    gpuMeshDesc meshDesc;

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
    scnDecal() {
        boxSize = gfxm::vec3(1.0f, 1.0f, 1.0f);
        float width = boxSize.x;
        float height = boxSize.y;
        float depth = boxSize.z;
        float w = width * .5f;
        float h = height * .5f;
        float d = depth * .5f;
        float vertices[] = {
            -w, -h,  d,     w,  h,  d,      w, -h,  d,
             w,  h,  d,    -w, -h,  d,     -w,  h,  d,

             w, -h,  d,     w,  h, -d,      w, -h, -d,
             w,  h, -d,     w, -h,  d,      w,  h,  d,

             w, -h, -d,    -w,  h, -d,     -w, -h, -d,
            -w,  h, -d,     w, -h, -d,      w,  h, -d,

            -w, -h, -d,    -w,  h,  d,     -w, -h,  d,
            -w,  h,  d,    -w, -h, -d,     -w,  h, -d,

            -w,  h,  d,     w,  h, -d,      w,  h,  d,
             w,  h, -d,    -w,  h,  d,     -w,  h, -d,

            -w, -h, -d,     w, -h,  d,      w, -h, -d,
             w, -h,  d,    -w, -h, -d,     -w, -h,  d
        };
        vertexBuffer.setArrayData(vertices, sizeof(vertices));
        meshDesc.setAttribArray(VFMT::Position_GUID, &vertexBuffer);
        meshDesc.setDrawMode(MESH_DRAW_TRIANGLES);
        meshDesc.setVertexCount(36);

        ubufDecal.reset(new gpuDecalUniformBuffer);
        ubufDecal->setSize(boxSize);
        ubufDecal->setColor(gfxm::vec4(1, 1, 1, 1));

        addRenderable(new gpuRenderable);
        getRenderable(0)->attachUniformBuffer(ubufDecal.get());
        getRenderable(0)->attachUniformBuffer(ubuf_model);
        getRenderable(0)->setMeshDesc(&meshDesc);
        setMaterial(resGet<gpuMaterial>("core/materials/decal.mat"));
    }

    void setMaterial(RHSHARED<gpuMaterial> material_) {
        this->material = material_;
        getRenderable(0)->setMaterial(material_.get());
    }
    RHSHARED<gpuMaterial> getMaterial() const {
        return this->material;
    }

    void setBoxSize(float x, float y, float z) {
        setBoxSize(gfxm::vec3(x, y, z));
    }
    void setBoxSize(const gfxm::vec3& boxSize) {
        this->boxSize = boxSize;
        ubufDecal->setSize(boxSize);

        float width = boxSize.x;
        float height = boxSize.y;
        float depth = boxSize.z;
        float w = width * .5f;
        float h = height * .5f;
        float d = depth * .5f;
        float vertices[] = {
            -w, -h,  d,     w,  h,  d,      w, -h,  d,
             w,  h,  d,    -w, -h,  d,     -w,  h,  d,

             w, -h,  d,     w,  h, -d,      w, -h, -d,
             w,  h, -d,     w, -h,  d,      w,  h,  d,

             w, -h, -d,    -w,  h, -d,     -w, -h, -d,
            -w,  h, -d,     w, -h, -d,      w,  h, -d,

            -w, -h, -d,    -w,  h,  d,     -w, -h,  d,
            -w,  h,  d,    -w, -h, -d,     -w,  h, -d,

            -w,  h,  d,     w,  h, -d,      w,  h,  d,
             w,  h, -d,    -w,  h,  d,     -w,  h, -d,

            -w, -h, -d,     w, -h,  d,      w, -h, -d,
             w, -h,  d,    -w, -h, -d,     -w, -h,  d
        };
        vertexBuffer.setArrayData(vertices, sizeof(vertices));
    }
    const gfxm::vec3& getBoxSize() const { return boxSize; }
    void setColor(const gfxm::vec4& rgba) {
        ubufDecal->setColor(rgba);
    }

    static void reflect() {
        type_register<scnDecal>("scnDecal")
            .parent<scnRenderObject>();
    }
};