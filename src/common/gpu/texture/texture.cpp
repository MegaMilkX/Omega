#include "texture.hpp"



void gpuTexture::_setDbgName(const std::string& name) {
    GL_CHECK(;);
    glObjectLabel(GL_TEXTURE, id, name.length(), name.data());
    GL_CHECK(;);
}