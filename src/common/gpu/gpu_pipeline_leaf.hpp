#pragma once

#include "gpu/gpu_pipeline_node.hpp"
#include "gpu/pass/gpu_pass.hpp"

class gpuPipelineLeaf : public gpuPipelineNode {
    std::unique_ptr<gpuPass> pass;
public:
    gpuPipelineLeaf(gpuPass* pass)
        : pass(pass) {}

    gpuPipelineNode* getChild(const std::string& name) {
        return nullptr;
    }
    gpuPass* getPass() { return pass.get(); }
    const gpuPass* getPass() const { return pass.get(); }

    int getPassList(gpuPass** passes, int max_count) override {
        return 0;
    }
    int getPassListImpl(gpuPass** passes, int offset, int max_count) override {
        passes[offset] = pass.get();
        return 1;
    }
    void enable(bool value) override {
        if(!pass) {
            assert(false);
            return;
        }
        pass->enable(value);
    }
};

