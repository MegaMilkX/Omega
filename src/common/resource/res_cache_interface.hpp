#pragma once

#include "handle/hshared.hpp"
#include "reflection/reflection.hpp"
#include <map>
#include "nlohmann/json.hpp"

class resCacheInterface {
public:
    virtual HSHARED_BASE* get(const char* name) = 0;
    virtual HSHARED_BASE* find(const char* name) = 0;
    virtual bool storeImpl(const char* name, const HSHARED_BASE*) = 0;
};

template<typename T>
class resCacheInterfaceT : public resCacheInterface {
public:
    bool storeImpl(const char* name, const HSHARED_BASE* ph) override {
        // TODO: Check type
        store(name, *(HSHARED<T>*)ph);
        return true; // ??
    }
    virtual void store(const char* name, HSHARED<T> h) = 0;
};

template<typename T>
class resCacheDefault : public resCacheInterfaceT<T> {
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
                LOG_ERR("Resource file " << name << " not found");
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
    virtual HSHARED_BASE* find(const char* name) override {
        auto it = objects.find(name);
        if (it == objects.end()) {
            return 0;
        }
        return &it->second;
    }
    virtual void store(const char* name, HSHARED<T> h) override {
        objects[name] = h;
    }
};