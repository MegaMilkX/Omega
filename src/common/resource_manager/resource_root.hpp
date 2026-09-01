#pragma once

#include "resource_root.auto.hpp"
#include <atomic>
#include <assert.h>
#include <type_traits>


[[cppi_decl, no_reflect]];
class PolymorphicResourceRootBase {
protected:
    PolymorphicResourceRootBase() = default;
    template<typename> friend class PolymorphicResourceRoot;
};

// A mixin that marks the first class deriving from it as a base of a resource family,
// Example:
//      class Texture : public PolymorphicResourceRoot<T> {};
//      class CubeTexture : public Texture {};
//      A createResource<CubeTexture>() call will go through the Texture's backend, not CubeTexture's
//      ResourceRef<CubeTexture> will also be convertible to ResourceRef<Texture>
// Constructor is private and T is friended so that only T can inherit PolymorphicResourceRoot<T>
// The static assert serves the same purpose, but keeping both guards just to show how important this is
[[cppi_tpl, no_reflect]];
template<typename T>
class PolymorphicResourceRoot : public PolymorphicResourceRootBase {
    friend T;
    PolymorphicResourceRoot() {
        static_assert(
            std::is_base_of_v<PolymorphicResourceRoot<T>, T>,
            "CRTP misuse: T must derive from PolymorphicResourceRoot<T>"
        );
    }
public:
    using ResourceRootType = T;
};


template<typename T, typename = void>
struct ResourceFamilyRoot { using type = T; };

template<typename T>
struct ResourceFamilyRoot<T, std::enable_if_t<std::is_base_of_v<PolymorphicResourceRootBase, T>>> {
    using type = typename T::ResourceRootType;
};

template<typename T>
using ResourceFamilyRoot_t = typename ResourceFamilyRoot<T>::type;


template<typename ROOT>
struct ResourceFamilyCounter {
    static inline std::atomic<uint32_t> next = 0;
};
template<typename ROOT, typename T>
uint32_t getResourceFamilyIndex() {
    static uint32_t idx = ResourceFamilyCounter<ROOT>::next++;
    assert(idx < sizeof(uint32_t));
    return idx;
}