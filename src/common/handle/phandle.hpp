#pragma once

#include <assert.h>
#include <unordered_map>
#include <stdint.h>
#include "block_storage.hpp"


union PHandleValue {
    struct {
        uint32_t index;
        uint32_t magic;
    };
    uint64_t handle = 0;

    PHandleValue() {}
    PHandleValue(uint64_t h) : handle(h) {}
    PHandleValue(uint32_t index, uint32_t magic) : index(index), magic(magic) {}
};


template<typename BASE_T>
class PHandleMgr {
    static constexpr uint32_t BLOCK_SIZE = 128;

    struct Data {
        BASE_T* object;
        uint32_t magic;
    };

    static BLOCK_STORAGE<Data, BLOCK_SIZE> storage;
    static std::vector<uint32_t> free_slots;
    static uint32_t next_magic;

public:
    static PHandleValue acquire(BASE_T* pobject) {
        if (!free_slots.empty()) {
            uint32_t index = free_slots.back();
            free_slots.pop_back();

            Data* pdata = storage.deref(index);
            pdata->object = pobject;
            // NOTE: magic already incremented on release
            return PHandleValue(index, pdata->magic);
        }

        uint32_t index = storage.add_one();
        uint32_t magic = ++next_magic;

        Data* pdata = storage.deref(index);
        pdata->object = pobject;
        pdata->magic = magic;
        return PHandleValue(index, magic);
    }
    static void release(PHandleValue h) {
        if (!isValid(h)) {
            assert(false);
            return;
        }
        Data* pdata = storage.deref(h.index);
        pdata->magic++;
        free_slots.push_back(h.index);
    }
    static bool isValid(PHandleValue h) {
        Data* pdata = storage.deref(h.index);
        if(!pdata) return false;
        return pdata->magic == h.magic;
    }

    static BASE_T* deref(PHandleValue h) {
        return storage.deref(h.index)->object;
    }

};
template<typename BASE_T>
BLOCK_STORAGE<typename PHandleMgr<BASE_T>::Data, PHandleMgr<BASE_T>::BLOCK_SIZE> PHandleMgr<BASE_T>::storage;
template<typename BASE_T>
std::vector<uint32_t> PHandleMgr<BASE_T>::free_slots;
template<typename BASE_T>
uint32_t PHandleMgr<BASE_T>::next_magic = 0;

template<typename BASE_T, typename T>
struct PHandle {
    PHandleValue value;

    PHandle() : value(0) {}
    PHandle(PHandleValue other) : value(other) {}
    PHandle(T* pobject) {
        acquire(pobject);
    }

    PHandle& operator=(PHandleValue other) {
        value = other;
        return *this;
    }

    bool operator==(const PHandle& other) const {
        return value.handle == other.value.handle;
    }

    void acquire(BASE_T* pobject);
    void release();
    bool isValid() const;
    operator bool() const;
    T*   deref();
    const T* deref() const;
    T* operator->();
    const T* operator->() const;
    bool operator<(const PHandle& other) const {
        return value.handle < other.value.handle;
    }
};
template<typename BASE_T, typename T>
struct std::hash<PHandle<BASE_T, T>> {
    size_t operator()(const PHandle<BASE_T, T>& k) const {
        return std::hash<uint64_t>().operator()(k.value.handle);
    }
};


template<typename BASE_T, typename T>
void PHandle<BASE_T, T>::acquire(BASE_T* pobject) {
    value = PHandleMgr<BASE_T>::acquire(pobject);
}
template<typename BASE_T, typename T>
void PHandle<BASE_T, T>::release() {
    PHandleMgr<BASE_T>::release(value);
}
template<typename BASE_T, typename T>
bool PHandle<BASE_T, T>::isValid() const {
    return PHandleMgr<BASE_T>::isValid(value);
}
template<typename BASE_T, typename T>
PHandle<BASE_T, T>::operator bool() const {
    return PHandleMgr<BASE_T>::isValid(value);
}
template<typename BASE_T, typename T>
T* PHandle<BASE_T, T>::deref() {
    if(!PHandleMgr<BASE_T>::isValid(value)) return nullptr;
    return static_cast<T*>(PHandleMgr<BASE_T>::deref(value));
}
template<typename BASE_T, typename T>
const T* PHandle<BASE_T, T>::deref() const {
    if(!PHandleMgr<BASE_T>::isValid(value)) return nullptr;
    return static_cast<T*>(PHandleMgr<BASE_T>::deref(value));
}
template<typename BASE_T, typename T>
T* PHandle<BASE_T, T>::operator->() {
    return deref();
}
template<typename BASE_T, typename T>
const T* PHandle<BASE_T, T>::operator->() const {
    return deref();
}
