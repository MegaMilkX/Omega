#pragma once

#include <map>
#include <memory>
#include "resource_manager/resource_backend.hpp"


class ColliderResourceBackend : public IResourceBackend {
    std::map<std::string, std::unique_ptr<ResourceEntry>> entries;
public:
    ColliderResourceBackend();
    ~ColliderResourceBackend();

    ResourceEntry* findEntry(const std::string&) override;
    ResourceEntry* createEntry(const std::string&) override;
    eResourceLoadResult load(ResourceEntry*) override;
    void release(void*) override;
    void collectGarbage() override;
};