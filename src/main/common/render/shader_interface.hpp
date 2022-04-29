#ifndef SHADER_INTERFACE_HPP
#define SHADER_INTERFACE_HPP

#include "common/math/gfxm.hpp"
#include "platform/gl/glextutil.h"
#include "glx_shader_program.hpp"
#include "common/log/log.hpp"

#include <vector>


template<typename T>
void setUniformValue(GLint location, const T& value) {
    static_assert(false, "type T is not supported");
}

template<>
inline void setUniformValue<float>(GLint location, const float& value) {
    glUniform1f(location, value);
}
template<>
inline void setUniformValue<int>(GLint location, const int& value) {
    glUniform1i(location, value);
}

template<>
inline void setUniformValue<gfxm::vec2>(GLint location, const gfxm::vec2& value) {
    glUniform2fv(location, 2, (float*)&value);
}

template<>
inline void setUniformValue<gfxm::vec3>(GLint location, const gfxm::vec3& value) {
    glUniform3fv(location, 3, (float*)&value);
}

template<>
inline void setUniformValue<gfxm::mat3>(GLint location, const gfxm::mat3& value) {
    glUniformMatrix3fv(location, 1, GL_FALSE, (float*)&value);
}

template<>
inline void setUniformValue<gfxm::mat4>(GLint location, const gfxm::mat4& value) {
    glUniformMatrix4fv(location, 1, GL_FALSE, (float*)&value);
}


template<typename UNIFORM>
class UNIFORM_INTERFACE_PART {
    GLint location;
public:
    UNIFORM_INTERFACE_PART(GLint location)
    : location(location) {}
    void setUniform(typename const UNIFORM::VALUE_T& value) {
        setUniformValue(location, value);
    }
};
template<typename Arg, typename... Args>
class UNIFORM_INTERFACE_IMPL;
template<typename Arg>
class UNIFORM_INTERFACE_IMPL<Arg> : public UNIFORM_INTERFACE_PART<Arg> {
public:
    UNIFORM_INTERFACE_IMPL()
    : UNIFORM_INTERFACE_PART<Arg>(-1)
    {}
    UNIFORM_INTERFACE_IMPL(gpuShaderProgram* program)
    : UNIFORM_INTERFACE_PART<Arg>(glGetUniformLocation(program->getId(), Arg::name))
    {}
};
template<typename Arg, typename... Args>
class UNIFORM_INTERFACE_IMPL  : public UNIFORM_INTERFACE_PART<Arg>, public UNIFORM_INTERFACE_IMPL<Args...> {
public:
    UNIFORM_INTERFACE_IMPL()
    : UNIFORM_INTERFACE_PART<Arg>(-1), UNIFORM_INTERFACE_IMPL<Args...>()
    {}
    UNIFORM_INTERFACE_IMPL(gpuShaderProgram* program)
    : UNIFORM_INTERFACE_PART<Arg>(glGetUniformLocation(program->getId(), Arg::name)), UNIFORM_INTERFACE_IMPL<Args...>(program)
    {}
};
template<typename... Args>
class UNIFORM_INTERFACE : public UNIFORM_INTERFACE_IMPL<Args...> {
public:
    UNIFORM_INTERFACE()
    : UNIFORM_INTERFACE_IMPL<Args...>() {}
    UNIFORM_INTERFACE(gpuShaderProgram* program)
    : UNIFORM_INTERFACE_IMPL<Args...>(program) 
    {}
    template<typename UNIFORM>
    void setUniform(typename const UNIFORM::VALUE_T& value) {
        UNIFORM_INTERFACE_PART<UNIFORM>::setUniform(value);
    }
};


#define DEF_UNIFORM(NAME, VALUE_TYPE) \
    class NAME { \
    public: \
        typedef VALUE_TYPE VALUE_T; \
        static constexpr char* name = #NAME; \
    };

#define DEF_UNIFORM_INTERFACE(NAME, ...) \
    typedef UNIFORM_INTERFACE<__VA_ARGS__> NAME;

DEF_UNIFORM(matModel,       gfxm::mat4);
DEF_UNIFORM(matView,        gfxm::mat4);
DEF_UNIFORM(matProjection,  gfxm::mat4);
DEF_UNIFORM(fTime,          float);
DEF_UNIFORM(lookupTextureWidth, int);

DEF_UNIFORM_INTERFACE(SHADER_INTERFACE_GENERIC,
    matModel,
    matView,
    matProjection,
    fTime,
    lookupTextureWidth // TODO: Only to test text, REMOVE
);


#endif
