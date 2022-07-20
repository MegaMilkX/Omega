#pragma once

#include "scn_render_object.hpp"

class scnMeshObject : public scnRenderObject {
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
    scnMeshObject() {
        

        addRenderable(new gpuRenderable);
        getRenderable(0)->attachUniformBuffer(ubuf_model);
    }
    void setMeshDesc(const gpuMeshDesc* desc) {
        getRenderable(0)->setMeshDesc(desc);
    }
    void setMaterial(gpuMaterial* mat) {
        getRenderable(0)->setMaterial(mat);
    }
};