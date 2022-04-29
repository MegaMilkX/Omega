#ifndef GL_ERROR_HPP
#define GL_ERROR_HPP

#include <assert.h>
#include <string>
#include <util/log.hpp>
#include <graphics/gl/glextutil.h>

namespace gl {

inline void checkError(const std::string& from) {
    auto err = glGetError();
    if(err) LOG(from << ": " << "GL error: " << err);
    assert(err == GL_NO_ERROR);
}

#define GL_LOG_ERROR(LABEL) ::gl::checkError(MKSTR(__FUNCTION__ << ", " << LABEL))

};

#endif
