#ifndef FRAME_FORMAT_HPP
#define FRAME_FORMAT_HPP

#include "platform/gl/glextutil.h"


namespace FFMT {

const int MAX_FRAME_LAYERS = 16;

struct LAYER_DESC {
    const char* name;
    const char* in_name;
    const char* out_name;
};

template<const char* STRING_NAME, const char* STRING_IN_NAME, const char* STRING_OUT_NAME>
struct LAYER {
    static const char* name;
    static const char* in_name;
    static const char* out_name;
};
template<const char* STRING_NAME, const char* STRING_IN_NAME, const char* STRING_OUT_NAME>
const char* LAYER<STRING_NAME, STRING_IN_NAME,  STRING_OUT_NAME>::name = STRING_NAME;
template<const char* STRING_NAME, const char* STRING_IN_NAME, const char* STRING_OUT_NAME>
const char* LAYER<STRING_NAME, STRING_IN_NAME, STRING_OUT_NAME>::in_name = STRING_IN_NAME;
template<const char* STRING_NAME, const char* STRING_IN_NAME, const char* STRING_OUT_NAME>
const char* LAYER<STRING_NAME, STRING_IN_NAME,  STRING_OUT_NAME>::out_name = STRING_OUT_NAME;

struct FRAME_DESC {
    int layerCount;
    LAYER_DESC layers[MAX_FRAME_LAYERS];
    bool hasDepth;
};


template<int N, typename Arg, typename... Args>
class FORMAT;

template<int N, typename Arg>
class FORMAT<N, Arg> {
    static const LAYER_DESC desc;
protected:
    static FRAME_DESC& generateFrameDesc(FRAME_DESC& d) {
        d.layers[N] = desc;
        d.layerCount = N + 1;
        return d;
    }
public:
    static const FRAME_DESC* getFrameDesc() {
        static FRAME_DESC desc = generateFrameDesc(desc);
        return &desc;
    }
};
template<int N, typename Arg>
const LAYER_DESC FORMAT<N, Arg>::desc = { Arg::name, Arg::in_name, Arg::out_name };

template<int N, typename Arg, typename... Args>
class FORMAT : public FORMAT<N + 1, Args...> {
    typedef FORMAT<N + 1, Args...> PARENT_T;
    static const LAYER_DESC desc;

protected:
    static FRAME_DESC& generateFrameDesc(FRAME_DESC& d) {
        d.layers[N] = desc;
        return PARENT_T::generateFrameDesc(d);
    }
public:
    static const FRAME_DESC* getFrameDesc() {
        static FRAME_DESC desc = generateFrameDesc(desc);
        return &desc;
    }
};
template<int N, typename Arg, typename... Args>
const LAYER_DESC FORMAT<N, Arg, Args...>::desc = { Arg::name, Arg::in_name, Arg::out_name };


#define DEF_LAYER_FMT(NAME, FMT, TYPE) \
    constexpr char NAME ## Name[] = #NAME; \
    constexpr char NAME ## InName[] = "in" #NAME; \
    constexpr char NAME ## OutName[] = "out" #NAME; \
    typedef LAYER<NAME ## Name, NAME ## InName, NAME ## OutName> NAME;
    
#define DEF_FRAME_FMT(NAME, ...) \
    struct ENUM_ ## NAME { enum { __VA_ARGS__ }; }; \
    typedef FORMAT<0, __VA_ARGS__> NAME;

DEF_LAYER_FMT(Albedo, GL_RGB, GL_UNSIGNED_BYTE);
DEF_LAYER_FMT(Normal, GL_RGB16F, GL_FLOAT);
DEF_LAYER_FMT(Metallic, GL_RED, GL_UNSIGNED_BYTE);
DEF_LAYER_FMT(Roughness, GL_RED, GL_UNSIGNED_BYTE);
DEF_LAYER_FMT(Lightness, GL_RGBA32F, GL_FLOAT);

DEF_FRAME_FMT(GBUFFER,
    Albedo,
    Normal,
    Metallic,
    Roughness,
    Lightness
);

}


#endif
