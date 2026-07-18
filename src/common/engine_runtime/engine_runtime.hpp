#pragma once

#include <unordered_map>
#include "reflection/reflection.hpp"

// The scheduler of updates and rendering
class IEngineRuntime {
    std::unordered_map<rtti::type, void*> components;
protected:
    template<typename T>
    void registerComponent(T* c) {
        registerComponent(rtti::type_get<T>(), c);
    }

    void registerComponent(rtti::type t, void* component) {
        components[t] = component;
    }

public:
    virtual ~IEngineRuntime() {}

    virtual void onDisplayChanged(int, int) {}
    virtual void run() = 0;

    template<typename T>
    T* getComponent() {
        auto it = components.find(rtti::type_get<T>());
        if (it != components.end()) {
            return reinterpret_cast<T*>(it->second);
        }
        return nullptr;
    }
};