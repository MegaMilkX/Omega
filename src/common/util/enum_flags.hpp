#pragma once

#include <type_traits>


template<typename T>
constexpr bool is_flag_enum_v = false;

#define ENUM_FLAGS(ENUM) \
    template<> constexpr bool is_flag_enum_v<ENUM> = true; \
    constexpr ENUM operator|(ENUM a, ENUM b) { \
        using U = std::underlying_type_t<ENUM>; \
        return static_cast<ENUM>(static_cast<U>(a) | static_cast<U>(b)); \
    } \
    constexpr ENUM operator&(ENUM a, ENUM b) { \
        using U = std::underlying_type_t<ENUM>; \
        return static_cast<ENUM>(static_cast<U>(a) & static_cast<U>(b)); \
    } \
    constexpr ENUM operator~(ENUM a) { \
        using U = std::underlying_type_t<ENUM>; \
        return static_cast<ENUM>(~static_cast<U>(a)); \
    } \
    constexpr ENUM& operator|=(ENUM& a, ENUM b) { return a = a | b; } \
    constexpr ENUM& operator&=(ENUM& a, ENUM b) { return a = a & b; }

template<typename F>
constexpr bool has_flags(F value, F flags) {
    using U = std::underlying_type_t<F>;
    return (static_cast<U>(value) & static_cast<U>(flags)) != 0;
}