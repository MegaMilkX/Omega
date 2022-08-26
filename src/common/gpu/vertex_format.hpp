#ifndef VERTEX_FORMAT_HPP
#define VERTEX_FORMAT_HPP

/*

    Helpers for managing 3d mesh vertex and vertex attribute formats

*/

#include "platform/gl/glextutil.h"

#include <assert.h>
#include <map>
#include <string>

namespace VFMT {

typedef int GUID;

#define VFMT_INVALID_GUID -1

template<int SIZE, GLenum GLTYPE>
struct ELEM_TYPE {
    static const int size = SIZE;
    static const GLenum gl_type = GLTYPE;
};

typedef ELEM_TYPE<1, GL_BYTE>           BYTE;
typedef ELEM_TYPE<1, GL_UNSIGNED_BYTE>  UBYTE;
typedef ELEM_TYPE<2, GL_SHORT>          SHORT;
typedef ELEM_TYPE<2, GL_UNSIGNED_SHORT> USHORT;
typedef ELEM_TYPE<2, GL_HALF_FLOAT>     HALF_FLOAT;
typedef ELEM_TYPE<4, GL_FLOAT>          FLOAT;
typedef ELEM_TYPE<4, GL_INT>            INT;
typedef ELEM_TYPE<4, GL_UNSIGNED_INT>   UINT;
typedef ELEM_TYPE<8, GL_DOUBLE>         DOUBLE;


struct ATTRIB_DESC {
    GUID         global_id;
    const char* name;
    const char* in_name;
    const char* out_name;
    int         elem_size;
    int         count;
    GLenum      gl_type;
    bool        normalized;
};

inline std::map<std::string, const ATTRIB_DESC*>& getAttribNameDescMap() {
    static std::map<std::string, const ATTRIB_DESC*> map_;
    return map_;
}
inline std::map<GUID, const ATTRIB_DESC*>& getAttribGUIDDescMap() {
    static std::map<GUID, const ATTRIB_DESC*> map_;
    return map_;
}
inline const ATTRIB_DESC* getAttribDesc(const char* name) {
    auto& map_ = getAttribNameDescMap();
    auto it = map_.find(name);
    if(it == map_.end()) {
        return 0;
    }
    return it->second;
}
inline const ATTRIB_DESC* getAttribDescWithInputName(const char* name) {
    if(strlen(name) < 2) {
        return 0;
    }
    return getAttribDesc(name + 2);
}
inline const ATTRIB_DESC* getAttribDesc(GUID global_id) {
    auto& map_ = getAttribGUIDDescMap();
    auto it = map_.find(global_id);
    if(it == map_.end()) {
        return 0;
    }
    return it->second;
}
inline ATTRIB_DESC createAttribDesc(ATTRIB_DESC desc, const ATTRIB_DESC* desc_record_ptr) {
    static GUID gid = 0;
    desc.global_id = gid++;
    getAttribNameDescMap()[desc.name] = desc_record_ptr;
    getAttribGUIDDescMap()[desc.global_id] = desc_record_ptr;
    return desc;
}

template<typename BASE_ELEM, int COUNT, bool NORMALIZED, const char* STRING_NAME, const char* STRING_IN_NAME, const char* STRING_OUT_NAME>
class ATTRIB {
public:
    static const GLenum gl_type     = BASE_ELEM::gl_type;
    static const int elem_size      = BASE_ELEM::size;
    static const int count          = COUNT;
    static const bool normalized     = NORMALIZED;
    static const char* name;
    static const char* in_name;
    static const char* out_name;
    static const ATTRIB_DESC desc;
};
template<typename BASE_ELEM, int COUNT, bool NORMALIZED, const char* STRING_NAME, const char* STRING_IN_NAME, const char* STRING_OUT_NAME>
const char* ATTRIB<BASE_ELEM, COUNT, NORMALIZED, STRING_NAME, STRING_IN_NAME,  STRING_OUT_NAME>::name = STRING_NAME;
template<typename BASE_ELEM, int COUNT, bool NORMALIZED, const char* STRING_NAME, const char* STRING_IN_NAME, const char* STRING_OUT_NAME>
const char* ATTRIB<BASE_ELEM, COUNT, NORMALIZED, STRING_NAME, STRING_IN_NAME, STRING_OUT_NAME>::in_name = STRING_IN_NAME;
template<typename BASE_ELEM, int COUNT, bool NORMALIZED, const char* STRING_NAME, const char* STRING_IN_NAME, const char* STRING_OUT_NAME>
const char* ATTRIB<BASE_ELEM, COUNT, NORMALIZED, STRING_NAME, STRING_IN_NAME,  STRING_OUT_NAME>::out_name = STRING_OUT_NAME;
template<typename BASE_ELEM, int COUNT, bool NORMALIZED, const char* STRING_NAME, const char* STRING_IN_NAME, const char* STRING_OUT_NAME>
const ATTRIB_DESC ATTRIB<BASE_ELEM, COUNT, NORMALIZED, STRING_NAME, STRING_IN_NAME,  STRING_OUT_NAME>::desc 
    = createAttribDesc(ATTRIB_DESC{ 
        0 /* set by the createAttribDesc() */, STRING_NAME, STRING_IN_NAME, STRING_OUT_NAME, BASE_ELEM::size, COUNT, BASE_ELEM::gl_type, NORMALIZED 
    }, &ATTRIB<BASE_ELEM, COUNT, NORMALIZED, STRING_NAME, STRING_IN_NAME,  STRING_OUT_NAME>::desc);


#define TYPEDEF_ATTRIB(ELEM, COUNT, NORMALIZED, NAME) \
    constexpr char NAME ## Name[] = #NAME; \
    constexpr char NAME ## InName[] = "in" #NAME; \
    constexpr char NAME ## OutName[] = "out" #NAME; \
    typedef ATTRIB<ELEM, COUNT, NORMALIZED, NAME ## Name, NAME ## InName, NAME ## OutName> NAME; \
    static const GUID& NAME ## _GUID = NAME::desc.global_id; \
    inline ATTRIB_DESC createDesc ## NAME() { \
        ATTRIB_DESC desc; \
        desc.name = NAME ## Name; \
        desc.in_name = NAME ## InName; \
        desc.out_name = NAME ## OutName; \
        desc.elem_size = NAME::elem_size; \
        desc.count = NAME::count; \
        desc.gl_type = NAME::gl_type; \
        desc.normalized = NAME::normalized; \
        return desc; \
    } \
    static const ATTRIB_DESC NAME ## Desc = createDesc ## NAME();

TYPEDEF_ATTRIB(FLOAT, 3, false, Position);
TYPEDEF_ATTRIB(FLOAT, 2, false, UV);
TYPEDEF_ATTRIB(FLOAT, 2, false, UVLightmap);
TYPEDEF_ATTRIB(FLOAT, 3, false, Normal);
TYPEDEF_ATTRIB(FLOAT, 3, false, Tangent);
TYPEDEF_ATTRIB(FLOAT, 3, false, Bitangent);
TYPEDEF_ATTRIB(FLOAT, 4, false, BoneIndex4);
TYPEDEF_ATTRIB(FLOAT, 4, false, BoneWeight4);
TYPEDEF_ATTRIB(UBYTE, 4, true,  ColorRGBA);
TYPEDEF_ATTRIB(UBYTE, 3, true,  ColorRGB);
TYPEDEF_ATTRIB(FLOAT, 3, false, Velocity);
TYPEDEF_ATTRIB(FLOAT, 1, false, TextUVLookup);

TYPEDEF_ATTRIB(FLOAT, 4, false, ParticlePosition);
TYPEDEF_ATTRIB(FLOAT, 4, false, ParticleData);
TYPEDEF_ATTRIB(FLOAT, 4, false, ParticleScale);
TYPEDEF_ATTRIB(FLOAT, 4, false, ParticleColorRGBA);
TYPEDEF_ATTRIB(FLOAT, 4, false, ParticleSpriteData);
TYPEDEF_ATTRIB(FLOAT, 4, false, ParticleSpriteUV);
TYPEDEF_ATTRIB(FLOAT, 4, false, ParticleRotation);

TYPEDEF_ATTRIB(FLOAT, 4, false, TrailInstanceData0);

const int MAX_VERTEX_ATTRIBS = 32;

struct VERTEX_DESC {
    int attribCount;
    const ATTRIB_DESC* attribs[MAX_VERTEX_ATTRIBS];

    int getLocalAttribID(GUID global_id) const {
        for(int i = 0; i < attribCount; ++i) {
            if(attribs[i]->global_id == global_id) {
                return i;
            }
        }
        return -1;
    }
};

template<int N, typename Arg, typename... Args>
class FORMAT;

template<int N, typename Arg>
class FORMAT<N, Arg> {
    static const ATTRIB_DESC* desc;

protected:
    static VERTEX_DESC& generateVertexDesc(VERTEX_DESC& d) {
        d.attribs[N] = desc;
        d.attribCount = N + 1;
        return d;
    }
public:
    static int vertexSize() {
        return Arg::elem_size * Arg::count;
    }

    static int attribCount() {
        return 1;
    }

    static const ATTRIB_DESC& getAttribDesc(int idx) {
        if(idx == N) {
            return desc;
        } else {
            assert(false);
            return desc;
        }
    }

    static void makeAttribNameArray(const char** names) {
        names[N] = desc.name;
    }

    static void makeOutAttribNameArray(const char** names) {
        names[N] = desc.out_name;
    }

    static const VERTEX_DESC* getVertexDesc() {
      static VERTEX_DESC desc = generateVertexDesc(desc);
      return &desc;
    }
};
template<int N, typename Arg>
const ATTRIB_DESC* FORMAT<N, Arg>::desc = &Arg::desc;

template<int N, typename Arg, typename... Args>
class FORMAT : public FORMAT<N + 1, Args...> {
    static const ATTRIB_DESC* desc;
    typedef FORMAT<N + 1, Args...> PARENT_T;

protected:
    static VERTEX_DESC& generateVertexDesc(VERTEX_DESC& d) {
        d.attribs[N] = desc;
        return PARENT_T::generateVertexDesc(d);
    }

public:
    static constexpr int attrib_count = sizeof...(Args) + 1;

    static int vertexSize() {
        return Arg::elem_size * Arg::count + PARENT_T::vertexSize();
    }

    static int attribCount() {
        return attrib_count;
    }

    static const ATTRIB_DESC& getAttribDesc(int idx) {
        if(idx == N) {
            return desc;
        } else {
            return PARENT_T::getAttribDesc(idx);
        }
    }

    static void makeAttribNameArray(const char** names) {
        names[N] = desc.name;
        PARENT_T::makeAttribNameArray(names);
    }

    static void makeOutAttribNameArray(const char** names) {
        names[N] = desc.out_name;
        PARENT_T::makeOutAttribNameArray(names);
    }

    static const VERTEX_DESC* getVertexDesc() {
        static VERTEX_DESC desc = generateVertexDesc(desc);
        return &desc;
    }
};
template<int N, typename Arg, typename... Args>
const ATTRIB_DESC* FORMAT<N, Arg, Args...>::desc = &Arg::desc;

#define DECL_VERTEX_FMT(NAME, ...) \
    struct ENUM_ ## NAME { enum { __VA_ARGS__ }; }; \
    typedef FORMAT<0, __VA_ARGS__> NAME;

DECL_VERTEX_FMT(GENERIC,
    Position, UV, UVLightmap,
    Normal, Tangent, Bitangent,
    BoneIndex4, BoneWeight4,
    ColorRGBA
);
DECL_VERTEX_FMT(PARTICLE,
    Position, Velocity, ColorRGBA
);
DECL_VERTEX_FMT(TEXT,
    Position, UV, TextUVLookup, ColorRGBA
);
DECL_VERTEX_FMT(QUAD_2D,
    Position, UV
);
DECL_VERTEX_FMT(LINE,
    Position, ColorRGBA
);

}

#endif

