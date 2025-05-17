#include "material_controller.hpp"


void MaterialController::setParam(const char* param_name, GPU_TYPE type, const void* pvalue) {
    for (auto n : model_nodes) {
        if (!n->getModelInstance()) {
            continue;
        }
        n->getModelInstance()->setParam(param_name, type, pvalue);
    }
}
void MaterialController::setFloat(const char* param_name, float value) {
    setParam(param_name, GPU_FLOAT, &value);
}
void MaterialController::setVec2(const char* param_name, const gfxm::vec2& value) {
    setParam(param_name, GPU_VEC2, &value);
}
void MaterialController::setVec3(const char* param_name, const gfxm::vec3& value) {
    setParam(param_name, GPU_VEC3, &value);
}
void MaterialController::setVec4(const char* param_name, const gfxm::vec4& value) {
    setParam(param_name, GPU_VEC4, &value);
}
void MaterialController::setMat3(const char* param_name, const gfxm::mat3& value) {
    setParam(param_name, GPU_MAT3, &value);
}
void MaterialController::setMat4(const char* param_name, const gfxm::mat4& value) {
    setParam(param_name, GPU_MAT4, &value);
}
