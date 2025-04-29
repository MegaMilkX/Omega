#pragma once

#include <string>


class gpuPass;
class gpuPipelineBranch;
class gpuPipelineNode {
    friend gpuPipelineBranch;

    gpuPipelineNode* parent = 0;
    int pass_count = 0; // recursive

    void increasePassCount() {
        ++pass_count;
        if (parent) {
            parent->increasePassCount();
        }
    }
public:
    virtual ~gpuPipelineNode() {}

    virtual gpuPipelineNode* getChild(const std::string& name) = 0;
    int getPassCount() { return pass_count; }
    virtual int getPassList(gpuPass** passes, int max_count) = 0;
    virtual int getPassListImpl(gpuPass** passes, int offset, int max_count) = 0;
    virtual void enable(bool value) = 0;
};