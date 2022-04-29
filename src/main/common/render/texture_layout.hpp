#ifndef TEXTURE_LAYOUT_HPP
#define TEXTURE_LAYOUT_HPP


namespace TL {

const int MAX_TEXTURE_LAYERS = 16;

enum DEFAULT_COLOR_MAP {
    DEFAULT_WHITE,
    DEFAULT_BLACK,
    DEFAULT_NORMAL,
    DEFAULT_COLOR_MAP_COUNT
};

struct LAYER_DESC {
    const char*         name;
    const char*         shader_name;
    DEFAULT_COLOR_MAP   default_color_map;
};

struct LAYOUT_DESC {
    int         layerCount;
    LAYER_DESC  layers[MAX_TEXTURE_LAYERS];
};

template<const char* STRING_NAME, const char* STRING_SHADER_NAME, DEFAULT_COLOR_MAP DEFAULT_COL_MAP>
struct LAYER {
    static const char* name;
    static const char* shader_name;
    static const DEFAULT_COLOR_MAP  default_color_map = DEFAULT_COL_MAP;
};
template<const char* STRING_NAME, const char* STRING_SHADER_NAME, DEFAULT_COLOR_MAP DEFAULT_COL_MAP>
const char* LAYER<STRING_NAME, STRING_SHADER_NAME, DEFAULT_COL_MAP>::name = STRING_NAME;
template<const char* STRING_NAME, const char* STRING_SHADER_NAME, DEFAULT_COLOR_MAP DEFAULT_COL_MAP>
const char* LAYER<STRING_NAME, STRING_SHADER_NAME, DEFAULT_COL_MAP>::shader_name = STRING_SHADER_NAME;


template<int N, typename Arg, typename... Args>
class LAYOUT;
template<int N, typename Arg>
class LAYOUT<N, Arg> {
    static const LAYER_DESC desc;
protected:
    static LAYOUT_DESC& createLayoutDesc(LAYOUT_DESC& d) {
        d.layers[N] = desc;
        d.layerCount = N + 1;
        return d;
    }
public:
    static const LAYOUT_DESC* getLayoutDesc() {
        static LAYOUT_DESC desc = createLayoutDesc(desc);
        return &desc;
    }
};
template<int N, typename Arg>
const LAYER_DESC LAYOUT<N, Arg>::desc = { Arg::name, Arg::shader_name, Arg::default_color_map };

template<int N, typename Arg, typename... Args>
class LAYOUT : public LAYOUT<N + 1, Args...> {
    typedef LAYOUT<N + 1, Args...> PARENT_T;
    static const LAYER_DESC desc;

protected:
    static LAYOUT_DESC& createLayoutDesc(LAYOUT_DESC& d) {
        d.layers[N] = desc;
        return PARENT_T::createLayoutDesc(d);
    }
public:
    static const LAYOUT_DESC* getLayoutDesc() {
        static LAYOUT_DESC desc = createLayoutDesc(desc);
        return &desc;
    }
};
template<int N, typename Arg, typename... Args>
const LAYER_DESC LAYOUT<N, Arg, Args...>::desc = { Arg::name, Arg::shader_name, Arg::default_color_map };

#define DEF_TEXTURE_LAYOUT_LAYER(NAME, DEFAULT_COLOR) \
    constexpr char NAME ## Name[] = #NAME; \
    constexpr char NAME ## ShaderName[] = "tex" #NAME; \
    typedef LAYER<NAME ## Name, NAME ## ShaderName, DEFAULT_COLOR> NAME;
#define DEF_TEXTURE_LAYOUT(NAME, ...) \
    struct NAME ## _ENUM { enum { __VA_ARGS__ }; }; \
    typedef LAYOUT<0, __VA_ARGS__> NAME;

DEF_TEXTURE_LAYOUT_LAYER(Albedo, DEFAULT_WHITE);
DEF_TEXTURE_LAYOUT_LAYER(Normal, DEFAULT_NORMAL);
DEF_TEXTURE_LAYOUT_LAYER(Metallic, DEFAULT_BLACK);
DEF_TEXTURE_LAYOUT_LAYER(Roughness, DEFAULT_BLACK);
DEF_TEXTURE_LAYOUT_LAYER(Emission, DEFAULT_BLACK);

DEF_TEXTURE_LAYOUT(GENERIC,
    Albedo,
    Normal,
    Metallic,
    Roughness,
    Emission
);

}


#endif
