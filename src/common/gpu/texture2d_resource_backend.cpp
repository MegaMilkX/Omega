#include "texture2d_resource_backend.hpp"

#include "resource_manager/resource_entry.hpp"
#include "gpu/gpu_texture_2d.hpp"


Texture2dResourceBackend::Texture2dResourceBackend() {
    is_running.store(1);
    reading_thread = std::thread([this]() {
        _th_reader();
    });
}
Texture2dResourceBackend::~Texture2dResourceBackend() {
    is_running.store(0);
    reading_thread.join();
}

ResourceEntry* Texture2dResourceBackend::findEntry(const std::string& resource_id) {
    auto it = entries.find(resource_id);
    if (it == entries.end()) {
        return nullptr;
    }
    return it->second.get();
}
ResourceEntry* Texture2dResourceBackend::createEntry(const std::string& resource_id) {
    auto it = entries.find(resource_id);
    if (it != entries.end()) {
        assert(false);
        return nullptr;
    }
    it = entries.insert(std::make_pair(resource_id, std::unique_ptr<ResourceEntry>(new TResourceEntry<gpuTexture2d>()))).first;
    return it->second.get();
}
void* Texture2dResourceBackend::load(ResourceEntry* rentry) {
    gpuTexture2d* tex = new gpuTexture2d;

    unsigned char col[4] = {0xFF, 0, 0xFF, 0xFF};
    tex->setData(col, 1, 1, 4, IMAGE_CHANNEL_UNSIGNED_BYTE);
    
    std::scoped_lock lock(read_queue_sync);
    READ_ENTRY& entry = read_queue.emplace_back();
    entry.tex = tex;
    entry.res_entry = rentry;
    return tex;
}
void* Texture2dResourceBackend::create() {
    return new gpuTexture2d();
}
void Texture2dResourceBackend::release(void* ptr) {
    delete static_cast<gpuTexture2d*>(ptr);
}
void Texture2dResourceBackend::collectGarbage() {
    for (auto& kv : entries) {
        auto entry = kv.second.get();
        if (entry->data != nullptr && entry->ref_count == 0) {
            entry->backend->release(entry->data);
            entry->data = nullptr;
            entry->state = eResourceUnloaded;
        }
    }
}
void Texture2dResourceBackend::update() {
    updateLoadedTextures();
}

void Texture2dResourceBackend::_th_reader() {
    while (is_running.load()) {
        read_queue_sync.lock();
        if (read_queue.empty()) {
            read_queue_sync.unlock();
            Sleep(15);
            continue;
        }

        READ_ENTRY entry;
        entry = std::move(read_queue.front());
        read_queue.erase(read_queue.begin());
        read_queue_sync.unlock();

        {
            byte_reader* reader = entry.res_entry->reader.get();
            auto view = reader->try_slurp();
            if (!view) {
                LOG_ERR("Failed to get data view from reader: '" << entry.res_entry->resource_id << "'");
                entry.res_entry->reader.reset();
                entry.res_entry->loading_payload.clear();
                continue;
            }

            entry.image.reset(new ktImage);
            if (!loadImage(entry.image.get(), view.data, view.size)) {
                assert(false);
                LOG_ERR("Failed to load image: '" << entry.res_entry->resource_id << "'");
                entry.image.reset();
                entry.res_entry->reader.reset();
                entry.res_entry->loading_payload.clear();
                continue;
            }
            entry.res_entry->reader.reset();
            entry.res_entry->loading_payload.clear();
        }

        loaded_queue_sync.lock();
        loaded_queue.push_back(std::move(entry));
        loaded_queue_sync.unlock();
    }
}

void Texture2dResourceBackend::updateLoadedTextures() {
    READ_ENTRY entry;
    {
        std::scoped_lock(loaded_queue_sync);
        if (loaded_queue.empty()) {
            return;
        }
        entry = std::move(loaded_queue.front());
        loaded_queue.erase(loaded_queue.begin());
    }

    entry.tex->setData(entry.image.get());
    entry.tex->generateMipmaps();
    //unsigned char col[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    //entry.tex->setData(col, 1, 1, 4, IMAGE_CHANNEL_UNSIGNED_BYTE);
}
