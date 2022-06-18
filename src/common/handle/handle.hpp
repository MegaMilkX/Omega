#pragma once

#include <assert.h>
#include <array>
#include <vector>
#include <string>
#include <stdint.h>
#include "log/log.hpp"

template<typename T>
struct Handle {
    union {
        struct {
            uint32_t index;
            uint32_t magic;
        };
        uint64_t     handle;
    };

    Handle<T>()
    : handle(0) {}
    Handle<T>(uint64_t other)
    : handle(other) {}
    Handle<T>& operator=(uint64_t other) {
        handle = other;
        return *this;
    }
};

template<typename T, int OBJECTS_PER_BLOCK>
class BLOCK_STORAGE {
    uint32_t element_count = 0;
    std::vector<std::array<unsigned char, OBJECTS_PER_BLOCK * sizeof(T)>> blocks;
public:
    T* deref(uint32_t index) {
        uint32_t lcl_id = index % OBJECTS_PER_BLOCK;
        uint32_t block_id = index / OBJECTS_PER_BLOCK;
        if (block_id >= blocks.size()) {
            return 0;
        }
        return (T*)&blocks[block_id][lcl_id * sizeof(T)];
    }
    // expand by 1 and return the last element pointer
    uint32_t add_one() {
        uint32_t index = element_count++;
        uint32_t lcl_id = index % OBJECTS_PER_BLOCK;
        uint32_t block_id = index / OBJECTS_PER_BLOCK;
        if (block_id == blocks.size()) {
            blocks.resize(blocks.size() + 1);
        }
        return index;
    }
};

constexpr uint32_t handle_block_size = 128;
template<typename T>
class HANDLE_MGR {
public:
    struct Data {
        T        object;
        uint32_t magic;
        std::string reference_name; // resource path or other hint for serialization
    };
private:
    static BLOCK_STORAGE<Data, handle_block_size> storage;
    static std::vector<uint32_t>   free_slots;
    static uint32_t                next_magic;
public:
    static Handle<T> acquire() {
        if (!free_slots.empty()) {
            uint32_t id = free_slots.back();
            free_slots.pop_back();
            Handle<T> h;
            h.index = id;
            h.magic = next_magic++;

            Data* ptr = storage.deref(id);
            new (ptr)(Data)();
            ptr->magic = h.magic;            
            //new (&ptr->object)(T)();
            return h;
        }
        uint32_t id = storage.add_one();
        Handle<T> h;
        h.index = id;
        h.magic = next_magic++;

        Data* ptr = storage.deref(id);
        new (ptr)(Data)();
        ptr->magic = h.magic;        
        //new (&ptr->object)(T)();
        return h;
    }
    static void release(Handle<T> h) {
        if (!isValid(h)) {
            assert(false);
            return;
        }
        Data* ptr = storage.deref(h.index);
        ptr->~Data();
        //ptr->object.~T();
        free_slots.emplace_back(h.index);
    }
    static bool isValid(Handle<T> h) {
        Data* ptr = storage.deref(h.index);
        if (!ptr) {
            return false;
        }
        return ptr->magic == h.magic;
    }
    static T*   deref(Handle<T> h) {
        return &storage.deref(h.index)->object;
    }
    static const std::string& getReferenceName(Handle<T> h) {
        if (!isValid(h)) {
            static std::string null_ref_name = "";
            return null_ref_name;
        }
        return storage.deref(h.index)->reference_name;
    }
    static void setReferenceName(Handle<T> h, const char* name) {
        if (!isValid(h)) {
            LOG_ERR("Attempted to name an object through an invalid handle");
            assert(false);
            return;
        }
        storage.deref(h.index)->reference_name = name;
    }
};
template<typename T>
BLOCK_STORAGE<typename HANDLE_MGR<T>::Data, handle_block_size> HANDLE_MGR<T>::storage;
template<typename T>
std::vector<uint32_t>                                HANDLE_MGR<T>::free_slots;
template<typename T>
uint32_t                                             HANDLE_MGR<T>::next_magic = 1;

