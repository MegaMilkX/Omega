#pragma once

#include <unordered_map>
#include "math/gfxm.hpp"
#include "reflection/reflection.hpp"


class modelComponent {
public:
    virtual ~modelComponent() {}
};

template<typename DATA_T>
class modelComponentT {
public:
};

struct modelMeshComponentData {

};
class modelMeshComponent : public modelComponentT<modelMeshComponentData> {
public:

};

// ================================


class modelNode {
    gfxm::vec3 translation;
    gfxm::quat rotation;
    gfxm::vec3 scale;
public:

};

class modelModel {
    modelNode root;
public:
    modelNode* getRoot() { return &root; }

    modelNode* createNode(modelNode* parent, const char* name = "node");
    void       destroyNode(modelNode* node);

    modelComponent* createComponent(modelNode* node, type t);
    modelComponent* getComponent(modelNode* node, type t);
    void            destroyComponent(modelNode* node, type t);

    template<typename T>
    T*              getComponentT(modelNode* node);
    template<typename T>
    T*              createComponentT(modelNode* node);
    template<typename T>
    void            destroyComponentT(modelNode* node);
    

};
