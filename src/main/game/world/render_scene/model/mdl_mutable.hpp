#pragma once

#include <memory>
#include "mdl_node.hpp"

struct mdlModelMutable {
    std::unique_ptr<mdlNode>  root;

    mdlModelMutable()
    : root(new mdlNode) {
        root->name = "Root";
    }

    mdlNode* getRoot() { return root.get(); }
};

