#include "texture.hpp"



void gpuTexture::_setDbgName(const std::string& name) {
    glObjectLabel(GL_TEXTURE, id, name.length(), name.data());
}