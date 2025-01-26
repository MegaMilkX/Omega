#include "static_model_instance.hpp"
#include "static_model.hpp"



StaticModelInstance::StaticModelInstance(StaticModel* static_model)
: static_model(static_model) {
    for (int i = 0; i < static_model->meshCount(); ++i) {
        auto mesh = static_model->getMesh(i);
        
        scnMeshObject* mo = new scnMeshObject;
        mo->setMeshDesc(mesh->getMeshDesc());
        mo->setTransformNode(transform);
        mo->setMaterial(static_model->getMaterial(static_model->getMaterialIndex(i)));
        
        mesh_objects.push_back(std::unique_ptr<scnMeshObject>(mo));
    }
}
StaticModelInstance::StaticModelInstance(StaticModelInstance&& other)
: static_model(other.static_model),
    transform(other.transform),
    mesh_objects(std::move(other.mesh_objects)) {

}
StaticModelInstance::~StaticModelInstance() {
    for (int i = 0; i < mesh_objects.size(); ++i) {
        auto& mo = mesh_objects[i];
        
    }
}

void StaticModelInstance::spawn(scnRenderScene* scn) {
    for (int i = 0; i < mesh_objects.size(); ++i) {
        auto& mo = mesh_objects[i];
        scn->addRenderObject(mo.get());
    }
}

void StaticModelInstance::despawn(scnRenderScene* scn) {
    for (int i = 0; i < mesh_objects.size(); ++i) {
        auto& mo = mesh_objects[i];
        scn->removeRenderObject(mo.get());
    }
}

