#pragma once

#include <unordered_map>
#include "reflection/reflection.hpp"
#include "param_block_mgr.hpp"
#include "param_block.hpp"


class gpuParamBlockContext {
    std::unordered_map<type, gpuParamBlockManager*> map;
public:
    // TODO: registering does not pass ownership, should it?
    template<typename PARAM_BLOCK_MGR_T>
    gpuParamBlockContext* registerParamBlock(gpuUniformBufferDesc* ubuf_desc, PARAM_BLOCK_MGR_T* mgr) {
        auto t = type_get<typename PARAM_BLOCK_MGR_T::block_t>();
        auto it = map.find(t);
        if (it != map.end()) {
            LOG_ERR("GPU parameter block manager for " << t.get_name() << " already exists");
            assert(false);
            return this;
        }
        mgr->init(ubuf_desc);
        map[type_get<typename PARAM_BLOCK_MGR_T::block_t>()] = mgr;
        return this;
    }
    template<typename PARAM_BLOCK_T>
    PARAM_BLOCK_T* createParamBlock() {
        auto it = map.find(type_get<PARAM_BLOCK_T>());
        if (it == map.end()) {
            assert(false);
            return nullptr;
        }
        return static_cast<PARAM_BLOCK_T*>(it->second->allocate());
    }
    void destroyParamBlock(gpuParamBlock* block) {
        block->getMgr()->release(block);
    }
    int update() {
        int n_uploaded = 0;
        for (auto kv : map) {
            n_uploaded += kv.second->upload();
        }
        return n_uploaded;
    }
};

