#include "material_controller.hpp"


void MaterialDriver::setParam(const char* param_name, GPU_TYPE type, const void* pvalue) {
    for (auto n : model_nodes) {
        if (!n->getModelInstance()) {
            continue;
        }
        n->getModelInstance()->setParam(param_name, type, pvalue);
    }
}
void MaterialDriver::setFloat(const char* param_name, float value) {
    setParam(param_name, GPU_FLOAT, &value);
}
void MaterialDriver::setVec2(const char* param_name, const gfxm::vec2& value) {
    setParam(param_name, GPU_VEC2, &value);
}
void MaterialDriver::setVec3(const char* param_name, const gfxm::vec3& value) {
    setParam(param_name, GPU_VEC3, &value);
}
void MaterialDriver::setVec4(const char* param_name, const gfxm::vec4& value) {
    setParam(param_name, GPU_VEC4, &value);
}
void MaterialDriver::setMat3(const char* param_name, const gfxm::mat3& value) {
    setParam(param_name, GPU_MAT3, &value);
}
void MaterialDriver::setMat4(const char* param_name, const gfxm::mat4& value) {
    setParam(param_name, GPU_MAT4, &value);
}
