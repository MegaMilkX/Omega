#pragma once

#include <assert.h>
#include <vector>
#include <stdint.h>

template<typename T>
struct Handle {
    union {
        struct {
            uint32_t index;
            uint32_t magic;
        };
        uint64_t     handle;
    };
};

template<typename T>
class HANDLE_MGR {
public:
    struct Data {
        T        object;
        uint32_t magic;
    };
private:
    
    static std::vector<Data>       data;
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
            data[id].magic = h.magic;
            data[id].object = T();
            return h;
        }
        uint32_t id = data.size();
        Handle<T> h;
        h.index = id;
        h.magic = next_magic++;
        data.resize(data.size() + 1);
        data.back().magic = h.magic;
        return h;
    }
    static void release(Handle<T> h) {
        if (!isValid(h)) {
            assert(false);
            return;
        }
        data[h.index].object.~T();
        free_slots.emplace_back(h.index);
    }
    static bool isValid(Handle<T> h) {
        return data[h.index].magic == h.magic;
    }
    static T*   deref(Handle<T> h) {
        return &data[h.index].object;
    }
};
template<typename T>
std::vector<typename HANDLE_MGR<T>::Data>            HANDLE_MGR<T>::data;
template<typename T>
std::vector<uint32_t>                                HANDLE_MGR<T>::free_slots;
template<typename T>
uint32_t                                             HANDLE_MGR<T>::next_magic = 1;

