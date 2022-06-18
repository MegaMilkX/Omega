#include "resource.hpp"

#include <assert.h>
#include <unordered_map>


static std::unordered_map<std::type_index, resCacheInterface*> caches;


bool resInit() {

    return true;
}

void resCleanup() {
    for (auto& it : caches) {
        delete it.second;
    }
}


void resAddCache(std::type_index type, resCacheInterface* iface) {
    assert(caches.find(type) == caches.end());

    caches.insert(std::make_pair(type, iface));
}

HSHARED_BASE* resGet(std::type_index type, const char* name) {
    assert(caches.find(type) != caches.end());

    auto& it = caches.find(type);
    auto cache = it->second;
    assert(cache);

    return cache->get(name);
}