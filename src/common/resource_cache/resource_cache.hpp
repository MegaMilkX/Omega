#pragma once

#include <string>
#include "handle/hshared.hpp"
#include "reflection/reflection.hpp"

template<typename T>
void resourceCacheStore(const std::string& name, const RHSHARED<T>& handle) {
    // TODO:
}

template<typename T>
RHSHARED<T> resourceCacheFind(const std::string& name) {
    // TODO:
    return RHSHARED<T>();
}