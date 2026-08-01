#pragma once

#include "resource_root.auto.hpp"

[[cppi_decl, no_reflect]];
class PolymorphicResourceRootBase {};

// A guard against calling loadResource<T> where T is a type derived from some resource,
// which would invoke the wrong resource backend
[[cppi_tpl, no_reflect]];
template<typename T>
class PolymorphicResourceRoot : public PolymorphicResourceRootBase {
    friend T;
    PolymorphicResourceRoot() = default;
public:
    using ResourceRootType = T;
};

