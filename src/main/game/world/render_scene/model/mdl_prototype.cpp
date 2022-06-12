#include "mdl_prototype.hpp"


void mdlModelPrototype::make(mdlModelMutable* mm) {
    std::vector<mdlNode*> nodes;

    auto root = mm->getRoot();
    int parent = -1;
    auto node = root;
    std::queue<mdlNode*> node_q;
    std::queue<int> parent_q;
    while (node) {
        for (int i = 0; i < node->childCount(); ++i) {
            auto child = node->getChild(i);
            node_q.push(child);
            parent_q.push(default_local_transforms.size());
        }

        name_to_node[node->name] = default_local_transforms.size();
        parents.push_back(parent);
        default_local_transforms.push_back(node->getLocalTransform());

        nodes.push_back(node);
        if (node_q.empty()) {
            node = 0;
        }
        else {
            node = node_q.front();
            node_q.pop();
            parent = parent_q.front();
            parent_q.pop();
        }
    }

    std::unordered_map<type, std::vector<mdlComponentMutable*>> mutables;
    for (int i = 0; i < nodes.size(); ++i) {
        auto n = nodes[i];
        LOG_WARN("mdlNode " << n->name);
        for (int j = 0; j < n->componentCount(); ++j) {
            auto t = n->getComponentType(j);
            auto c = n->getComponent(t);

            auto it = components.find(t);
            if (it == components.end()) {
                it = components.insert(
                    std::make_pair(t, std::unique_ptr<mdlComponentPrototype>(mdlComponentGetTypeDesc(t)->pfn_constructPrototype()))
                ).first;
            }

            mutables[t].push_back(c);
        }
    }
    for (auto& kv : mutables) {
        auto it = components.find(kv.first);
        if (it == components.end()) {
            it = components.insert(
                std::make_pair(kv.first, std::unique_ptr<mdlComponentPrototype>(mdlComponentGetTypeDesc(kv.first)->pfn_constructPrototype()))
            ).first;
        }
        it->second->make(this, kv.second.data(), kv.second.size());
    }
}