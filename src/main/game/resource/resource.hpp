#pragma once

#include <assert.h>
#include <typeinfo>
#include <typeindex>

#include "res_cache_interface.hpp"

bool resInit();
void resCleanup();


template<typename T>
void resAddCache(resCacheInterface* iface) {
    void resAddCache(std::type_index type, resCacheInterface* iface);
    resAddCache(typeid(T), iface);
}

template<typename T>
T* resGet(const char* name) {
    void* resGet(std::type_index type, const char* name);
    // TODO: should at least do a type size check
    return (T*)resGet(typeid(T), name);
}