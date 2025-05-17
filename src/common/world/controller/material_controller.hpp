#include "material_controller.auto.hpp"
#include <set>
#include "actor_controller.hpp"
#include "world/node/node_skeletal_model.hpp"


[[cppi_class]];
class MaterialController : public ActorController {
    std::set<SkeletalModelNode*> model_nodes;
public:
    TYPE_ENABLE();

    void enableTechnique(const char* path, bool value) {
        for (auto n : model_nodes) {
            if (!n->getModelInstance()) {
                continue;
            }
            n->getModelInstance()->enableTechnique(path, value);
        }
    }

    void setParam(const char* param_name, GPU_TYPE type, const void* pvalue);
    void setFloat(const char* param_name, float value);
    void setVec2(const char* param_name, const gfxm::vec2& value);
    void setVec3(const char* param_name, const gfxm::vec3& value);
    void setVec4(const char* param_name, const gfxm::vec4& value);
    void setMat3(const char* param_name, const gfxm::mat3& value);
    void setMat4(const char* param_name, const gfxm::mat4& value);


    void onReset() override {
    
    }
    void onSpawn(Actor* actor) override {

    }
    void onDespawn(Actor* actor) override {

    }
    void onActorNodeRegister(type t, ActorNode* node, const std::string& name) override {
        if (t == type_get<SkeletalModelNode>()) {
            model_nodes.insert((SkeletalModelNode*)node);
        }
    }
    void onActorNodeUnregister(type t, ActorNode* node, const std::string& name) override {
        if (t == type_get<SkeletalModelNode>()) {
            model_nodes.erase((SkeletalModelNode*)node);
        }
    }
    void onUpdate(RuntimeWorld* world, float dt) {

    }
};