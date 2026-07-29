#pragma once


class PolymorphicResourceRootBase {};

// A guard against calling loadResource<T> where T is a type derived from some resource,
// which would invoke the wrong resource backend
template<typename T>
class PolymorphicResourceRoot : public PolymorphicResourceRootBase {
    friend T;
    PolymorphicResourceRoot() = default;
public:
    using ResourceRootType = T;
};

