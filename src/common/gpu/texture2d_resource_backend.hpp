#pragma once

#include <thread>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include "resource_manager/resource_backend.hpp"
#include "gpu/gpu_texture_2d.hpp"


class Texture2dResourceBackend : public IResourceBackend {
    std::thread reading_thread;
    std::atomic<int> is_running;

    std::map<std::string, std::unique_ptr<ResourceEntry>> entries;
    
    struct READ_ENTRY {
        gpuTexture2d* tex = nullptr;
        ResourceEntry* res_entry = nullptr;
        std::unique_ptr<ktImage> image;
    };
    std::vector<READ_ENTRY> read_queue;
    std::mutex read_queue_sync;

    std::vector<READ_ENTRY> loaded_queue;
    std::mutex loaded_queue_sync;

    GLuint pbo = 0;
public:
    Texture2dResourceBackend();
    ~Texture2dResourceBackend();
    ResourceEntry* findEntry(const std::string&) override;
    ResourceEntry* createEntry(const std::string&) override;
    void* load(ResourceEntry*) override;
    void* create() override;
    void release(void*) override;
    void collectGarbage() override;
    void update() override;

    void _th_reader();

    void updateLoadedTextures();
};

