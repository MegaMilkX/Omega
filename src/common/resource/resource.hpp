#pragma once

#include <assert.h>
#include <typeinfo>
#include <typeindex>

#include "res_cache_interface.hpp"

#include "handle/hshared.hpp"

bool resInit();
void resCleanup();


template<typename T>
void resAddCache(resCacheInterface* iface) {
    void resAddCache(std::type_index type, resCacheInterface* iface);
    resAddCache(typeid(T), iface);
}
/*
template<typename T>
T* resGet(const char* name) {
    void* resGet(std::type_index type, const char* name);
    // TODO: should at least do a type size check
    return (T*)resGet(typeid(T), name);
}*/
template<typename T>
HSHARED<T> resGet(const char* name) {
    HSHARED_BASE* resGet(std::type_index type, const char* name);
    // TODO: should at least do a type size check
    HSHARED_BASE* phs_base = resGet(typeid(T), name);
    HSHARED<T>* phs = dynamic_cast<HSHARED<T>*>(phs_base);
    if (phs) {
        return *phs;
    } else {
        return HSHARED<T>(0);
    }
}