#ifndef GLX_BUFFER_HPP
#define GLX_BUFFER_HPP

#include "platform/gl/glextutil.h"


class gpuBuffer {
    size_t size = 0;
    GLuint id   = 0;
public:
    gpuBuffer() {
        glGenBuffers(1, &id);
    }
    ~gpuBuffer() {
        glDeleteBuffers(1, &id);
    }
    GLuint getId() const { 
        return id; 
    }
    size_t getSize() const {
        return size;
    }
    void reserve(size_t size, GLenum usage) {
        this->size = size;
        glBindBuffer(GL_ARRAY_BUFFER, id);
        glBufferData(GL_ARRAY_BUFFER, size, 0, usage);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    void setArrayData(const void* data, size_t size) {
        this->size = size;
        glBindBuffer(GL_ARRAY_BUFFER, id);
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    void setArraySubData(const void* data, size_t size, size_t offset) {
        glBindBuffer(GL_ARRAY_BUFFER, id);
        glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
        glBindBuffer(GL_ARRAY_BUFFER, id);
    }
    void bindArray() const {
        glBindBuffer(GL_ARRAY_BUFFER, id);
    }
    void bindIndexArray() const {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    }
    void bindUniform() const {
        glBindBuffer(GL_UNIFORM_BUFFER, id);
    }

    void getData(void* target) const {
        glBindBuffer(GL_ARRAY_BUFFER, id);
        glGetBufferSubData(GL_ARRAY_BUFFER, 0, size, target);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
};


#endif
