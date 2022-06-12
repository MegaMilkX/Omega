#pragma once

#include "mdl_type.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include "math/gfxm.hpp"

#include "mdl_component_mutable.hpp"

struct mdlNode {
    mdlNode* parent = 0;
    std::string name;
    gfxm::vec3 translation;
    gfxm::quat rotation;
    gfxm::vec3 scale = gfxm::vec3(1.f, 1.f, 1.f);
    gfxm::mat4 world = gfxm::mat4(1.0f);
    std::vector<std::unique_ptr<mdlNode>> children;
    std::unordered_map<type, std::unique_ptr<mdlComponentMutable>> components;

    gfxm::mat4 getLocalTransform() {
        return gfxm::translate(gfxm::mat4(1.0f), translation)
            * gfxm::to_mat4(rotation)
            * gfxm::scale(gfxm::mat4(1.0f), scale);
    }
    const gfxm::mat4& getWorldTransform() {
        return world = (parent ? parent->getWorldTransform() * getLocalTransform() : getLocalTransform());
    }

    mdlNode* createChild(const char* name) {
        mdlNode* n = new mdlNode();
        n->name = name;
        n->parent = this;
        children.push_back(std::unique_ptr<mdlNode>(n));
        return n;
    }
    size_t childCount() const {
        return children.size();
    }
    mdlNode* getChild(int i) {
        return children[i].get();
    }
    template<typename T>
    typename T::mutable_t* createComponent() {
        if (components.find(type_get<T>()) != components.end()) {
            assert(false);
            return 0;
        }
        auto p = new typename T::mutable_t;
        p->owner = this;
        components.insert(std::make_pair(type_get<T>(), std::unique_ptr<mdlComponentMutable>(p)));
        return p;
    }
    size_t componentCount() const {
        return components.size();
    }
    template<typename T>
    typename T::mutable_t* getComponent() {
        auto it = components.find(type_get<T>());
        if (it == components.end()) {
            assert(false);
            return 0;
        }
        return it->second.get();
    }
    type getComponentType(int i) {
        auto it = components.begin();
        std::advance(it, i);
        return it->first;
    }
    mdlComponentMutable* getComponent(type t) {
        auto it = components.find(t);
        if (it == components.end()) {
            assert(false);
            return 0;
        }
        return it->second.get();
    }

};