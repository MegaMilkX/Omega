#pragma once

#include <assert.h>
#include <set>

template<typename T>
class GuidPool {
    static std::set<int> free_ids;
    static int next_id;
public:
    static int AllocId() {
        int id = 0;
        if (free_ids.empty()) {
            id = next_id++;
        } else {
            id = *free_ids.begin();
            free_ids.erase(id);
        }
        return id;
    }
    static void FreeId(int id) {
        assert(free_ids.find(id) == free_ids.end());
        free_ids.insert(id);
    }
};

template<typename T>
std::set<int> GuidPool<T>::free_ids;
template<typename T>
int GuidPool<T>::next_id = 0;