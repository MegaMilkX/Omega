#pragma once

#include <assert.h>
#include <vector>
#include <type_traits>
#include "reflection/reflection.hpp"
#include "gpu/gpu_uniform_buffer.hpp"


struct gpuParamBlock;
class gpuRenderable;
class gpuParamBlockManager {
protected:
    struct ITEM {
        gpuParamBlock* block = nullptr;
        void* internal = nullptr;
        gpuUniformBuffer* gpu_buf = nullptr;
    };

    virtual gpuParamBlock* allocateBlock() = 0;
    virtual void* allocateInternalBlock() = 0;
    virtual void releaseBlock(gpuParamBlock* block) = 0;
    virtual void releaseInternalBlock(void* inter) = 0;

    gpuUniformBufferDesc* ubuf_desc = nullptr;
    std::vector<ITEM> items;
    size_t dirty_count = 0;
public:
    virtual ~gpuParamBlockManager() {}

    bool init(gpuUniformBufferDesc* ubuf_desc);
    
    gpuParamBlock* allocate();
    void release(gpuParamBlock* b);
    
    void markDirty(gpuParamBlock* b);

    void attach(gpuRenderable* renderable, gpuParamBlock* block);

    int upload();

    virtual type getBlockType() const = 0;
    virtual void onInit() = 0;
    virtual void upload(ITEM* items, size_t count) = 0;
};

template<typename PARAM_BLOCK_T, typename INTERNAL_BLOCK_T = void>
class gpuParamBlockMgr_T : public gpuParamBlockManager {
protected:
    PARAM_BLOCK_T* getBlock(ITEM& item) {
        return static_cast<PARAM_BLOCK_T*>(item.block);
    }
    INTERNAL_BLOCK_T* getInternal(ITEM& item) {
        if constexpr (!std::is_void_v<INTERNAL_BLOCK_T>) {
            return static_cast<INTERNAL_BLOCK_T*>(item.internal);
        } else {
            return nullptr;
        }
    }

private:
    gpuParamBlock* allocateBlock() override {
        return new PARAM_BLOCK_T;
    }
    void* allocateInternalBlock() override {
        if constexpr (!std::is_void_v<INTERNAL_BLOCK_T>) {
            return new INTERNAL_BLOCK_T;
        } else {
            return nullptr;
        }
    }
    void releaseBlock(gpuParamBlock* block) override {
        delete block;
    }
    void releaseInternalBlock(void* inter) override {
        if constexpr (!std::is_void_v<INTERNAL_BLOCK_T>) {
            delete (INTERNAL_BLOCK_T*)inter;
        }
    }

public:
    using block_t = PARAM_BLOCK_T;
    using internal_block_t = INTERNAL_BLOCK_T;

    type getBlockType() const override { return type_get<PARAM_BLOCK_T>(); };
};

