#pragma once

#include <map>
#include <memory>
#include "resource_manager/resource_backend.hpp"


class MaterialResourceBackend : public IResourceBackend {
    std::map<std::string, std::unique_ptr<ResourceEntry>> entries;
public:
    MaterialResourceBackend();
    ~MaterialResourceBackend() {}

    ResourceEntry* findEntry(const std::string&) override;
    ResourceEntry* createEntry(const std::string&) override;
    void* load(ResourceEntry*) override;
    void release(void*) override;
    void collectGarbage() override;
};