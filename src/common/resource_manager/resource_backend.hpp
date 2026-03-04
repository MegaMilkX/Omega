#pragma once

#include "byte_reader/byte_reader.hpp"


struct ResourceEntry;
class IResourceBackend {
public:
    virtual ~IResourceBackend() {}
    virtual ResourceEntry* findEntry(const std::string&) = 0;
    virtual ResourceEntry* createEntry(const std::string&) = 0;
    virtual void* load(byte_reader&) = 0;
    virtual void* create() = 0;
    virtual void release(void*) = 0;
    virtual void collectGarbage() = 0;
};