#pragma once

#include <map>
#include <memory>
#include "animation/curve.hpp"


class animModelComponentNode {
public:
    std::string name;
    
    virtual ~animModelComponentNode() {}
    
    virtual void sample(void* sample_ptr, float cursor) = 0;
};
template<typename SAMPLE_TYPE>
class animModelComponentNodeT : public animModelComponentNode {
public:
    using SAMPLE_T = SAMPLE_TYPE;

    void sample(void* sample_ptr, float cursor) override {
        onSample((SAMPLE_T*)sample_ptr, cursor);
    }

    virtual void onSample(SAMPLE_T* sample, float cursor) = 0;
};


struct animDecalSample {
    gfxm::vec4 rgba;
};
class animDecalNode : public animModelComponentNodeT<animDecalSample> {
public:
    curve<gfxm::vec4> rgba;
    void onSample(animDecalSample* s, float cursor) override {
        s->rgba = rgba.at(cursor, gfxm::vec4(1, 1, 1, 1));
    }
};


struct animModelSampleBuffer {
    std::vector<char> buffer;

    void* operator[](int offset) {
        return &buffer[offset];
    }
};
class animModelAnimMapping {
    std::vector<int32_t> mapping;
public:
    void resize(int sz) { mapping.resize(sz); }

    const int32_t& operator[](int idx) const {
        return mapping[idx];
    }
    int32_t& operator[](int idx) {
        return mapping[idx];
    }
};

class animModelSequence {
    std::vector<std::unique_ptr<animModelComponentNode>> nodes;
    std::map<std::string, int> node_name_to_index;
public:
    float length = .0f;
    float fps = 60.0f;

    template<typename T>
    T* createNode(const char* name) {
        if (node_name_to_index.find(name) != node_name_to_index.end()) {
            assert(false);
            return 0;
        }
        T* ptr = new T();
        ptr->name = name;
        node_name_to_index[name] = nodes.size();
        nodes.emplace_back(std::unique_ptr<animModelComponentNode>(ptr));
        return ptr;
    }
    int nodeCount() const {
        return nodes.size();
    }
    animModelComponentNode* getNode(int idx) {
        return nodes[idx].get();
    }

    void sample_remapped(
        animModelSampleBuffer* samples,
        float cursor,
        const animModelAnimMapping& mapping
    ) {
        for (size_t i = 0; i < nodes.size(); ++i) {
            auto& n = nodes[i];
            int32_t offset = mapping[i];
            if (offset == -1) {
                continue;
            }
            //assert(offset >= 0 && offset < sample_count);
            void* sample_ptr = samples->operator[](offset);
            n->sample(sample_ptr, cursor);
        }
    }
};