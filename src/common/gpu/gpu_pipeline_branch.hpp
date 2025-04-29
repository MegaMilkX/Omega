#pragma once

#include <map>
#include "gpu_pipeline_node.hpp"
#include "gpu_pipeline_leaf.hpp"


class gpuPipelineBranch : public gpuPipelineNode {
    std::map<std::string, std::unique_ptr<gpuPipelineNode>> children;

public:
    void addChild(const std::string& name, gpuPipelineNode* ch) {
        ch->parent = this;

        children.insert(
            std::make_pair(
                name, std::unique_ptr<gpuPipelineNode>(ch)
            )
        );
    }
    gpuPipelineBranch* getOrCreateBranch(const std::string& name) {
        auto it = children.find(name);
        if (it != children.end()) {
            gpuPipelineBranch* branch
                = dynamic_cast<gpuPipelineBranch*>(it->second.get());
            if (!branch) {
                LOG_ERR(name << " already exists and is not a branch");
                assert(false);
                return 0;
            }
            return branch;
        }

        gpuPipelineBranch* branch = new gpuPipelineBranch;
        branch->parent = this;
        children.insert(
            std::make_pair(
                name, std::unique_ptr<gpuPipelineNode>(branch)
            )
        );
        return branch;
    }
    gpuPipelineNode* getChild(const std::string& name) override {
        auto it = children.find(name);
        if (it == children.end()) {
            return 0;
        }
        return it->second.get();
    }
    const gpuPipelineBranch* getBranch(const std::string& name) const {
        auto it = children.find(name);
        if (it == children.end()) {
            return 0;
        }

        const gpuPipelineBranch* branch
            = dynamic_cast<gpuPipelineBranch*>(it->second.get());
        return branch;
    }
    const gpuPass* getPass(const std::string& name) const {
        auto it = children.find(name);
        if (it == children.end()) {
            return 0;
        }

        const gpuPipelineLeaf* leaf
            = dynamic_cast<gpuPipelineLeaf*>(it->second.get());
        if (!leaf) {
            return 0;
        }
        return leaf->getPass();
    }
    gpuPass* addPass(const std::string& name, gpuPass* pass) {
        auto it = children.find(name);
        if (it != children.end()) {
            gpuPipelineLeaf* leaf
                = dynamic_cast<gpuPipelineLeaf*>(it->second.get());
            if (leaf) {
                LOG_ERR("Pass " << name << " already exists");
                assert(false);
                return 0;
            }
            LOG_ERR("Technique " << name << " already exists");
            assert(false);
            return 0;
        }
        auto leaf = new gpuPipelineLeaf(pass);
        leaf->parent = this;
        children.insert(
            std::make_pair(
                name, std::unique_ptr<gpuPipelineNode>(leaf)
            )
        );

        increasePassCount();
        return pass;
    }

    int getPassList(gpuPass** passes, int max_count) override {
        return getPassListImpl(passes, 0, max_count);
    }
    int getPassListImpl(gpuPass** passes, int offset, int max_count) override {
        int offset_next = offset;
        for (auto& kv : children) {
            int count = kv.second->getPassListImpl(passes, offset_next, max_count);
            offset_next += count;
        }
        return offset_next - offset;
    }
    void enable(bool value) override {
        for (auto& kv : children) {
            kv.second->enable(value);
        }
    }
};