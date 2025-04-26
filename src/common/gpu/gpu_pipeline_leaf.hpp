#pragma once

#include "gpu/gpu_pipeline_node.hpp"
#include "gpu/pass/gpu_pass.hpp"

class gpuPipelineLeaf : public gpuPipelineNode {
    std::unique_ptr<gpuPass> pass;
public:
    gpuPipelineLeaf(gpuPass* pass)
        : pass(pass) {}

    gpuPass* getPass() { return pass.get(); }
    const gpuPass* getPass() const { return pass.get(); }
};

