#include "resource.hpp"

#include <assert.h>
#include <unordered_map>


static std::unordered_map<std::type_index, resCacheInterface*>& getCaches() {
    static std::unordered_map<std::type_index, resCacheInterface*> caches;
    return caches;
}

bool resInit() {

    return true;
}

void resCleanup() {
    for (auto& it : getCaches()) {
        delete it.second;
    }
}


void resAddCache(std::type_index type, resCacheInterface* iface) {
    assert(getCaches().find(type) == getCaches().end());

    getCaches().insert(std::make_pair(type, iface));
}

HSHARED_BASE* resGet(std::type_index type, const char* name) {
    assert(getCaches().find(type) != getCaches().end());

    auto& it = getCaches().find(type);
    auto cache = it->second;
    assert(cache);

    return cache->get(name);
}