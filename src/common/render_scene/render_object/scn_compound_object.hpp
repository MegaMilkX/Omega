#pragma once

#include "scn_render_object.hpp"

#include <unordered_set>

class scnCompoundObject : public scnRenderObject {
    std::unordered_set<scnRenderObject*> objects;

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
    scnCompoundObject() {}
    ~scnCompoundObject() {}

    void addObject(scnRenderObject* obj) {

    }
    void removeObject(scnRenderObject* obj) {

    }
};
