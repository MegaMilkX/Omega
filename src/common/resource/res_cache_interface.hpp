#pragma once

#include "handle/hshared.hpp"
#include "reflection/reflection.hpp"
#include <map>

class resCacheInterface {
public:
    virtual HSHARED_BASE* get(const char* name) = 0;
};


template<typename T>
class resCacheDefault : public resCacheInterface {
    std::map<std::string, HSHARED<T>> objects;
public:
    resCacheDefault() {}

    HSHARED_BASE* get(const char* name) override {
        auto it = objects.find(name);
        if (it == objects.end()) {
            Handle<T> handle = HANDLE_MGR<T>::acquire();

            std::ifstream f(name);
            if (!f) {
                HANDLE_MGR<T>::release(handle);
                return 0;
            }
            nlohmann::json j;
            f >> j;
            type_get<T>().deserialize_json(j, HANDLE_MGR<T>::deref(handle));

            it = objects.insert(std::make_pair(std::string(name), HSHARED<T>(handle))).first;
            it->second.setReferenceName(name);
        }
        return &it->second;
    }
};