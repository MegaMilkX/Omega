#pragma once

#include <assert.h>
#include <typeinfo>
#include <typeindex>

#include "log/log.hpp"
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
    bool hasCache(std::type_index type);

    if (!hasCache(typeid(T))) {
        const std::type_info& ti = typeid(T);
        LOG_WARN("Adding default cache for " << ti.name());
        resAddCache<T>(new resCacheDefault<T>());
    }

    // TODO: should at least do a type size check
    HSHARED_BASE* phs_base = resGet(typeid(T), name);
    HSHARED<T>* phs = dynamic_cast<HSHARED<T>*>(phs_base);
    if (phs) {
        return *phs;
    } else {
        return HSHARED<T>(0);
    }
}

template<typename T>
void resStore(const char* name, const RHSHARED<T>& h) {
    bool resStore(const char* name, std::type_index type, const HSHARED_BASE* ptr);

    resStore(name, typeid(T), &h);
}

template<typename T>
HSHARED<T> resFind(const char* name) {
    HSHARED_BASE* resFind(std::type_index type, const char* name);
    bool hasCache(std::type_index type);
    if (!hasCache(typeid(T))) {
        const std::type_info& ti = typeid(T);
        LOG_WARN("Adding default cache for " << ti.name());
        resAddCache<T>(new resCacheDefault<T>());
    }
    
    HSHARED_BASE* phs_base = resFind(typeid(T), name);
    HSHARED<T>* phs = dynamic_cast<HSHARED<T>*>(phs_base);
    if (phs) {
        return *phs;
    } else {
        return HSHARED<T>(0);
    }
}