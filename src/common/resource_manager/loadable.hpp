#pragma once

#include <span>
#include "byte_reader/byte_reader.hpp"


using extension_list = std::span<const extension>;
template<typename T>
concept has_static_get_extensions = requires {
    { T::get_extensions() } -> std::convertible_to<extension_list>;
};

#define DEFINE_EXTENSIONS(...) \
    static extension_list get_extensions() { \
        static constexpr extension exts[] = { \
            __VA_ARGS__ \
        }; \
        return exts; \
    }

[[cppi_decl, no_reflect]];
class ILoadable;

class ILoadable {
public:
    virtual ~ILoadable() {}

    virtual bool load(byte_reader&) = 0;
};