#include "model.hpp"

#include "mdl_type.hpp"
#include "components/mdl_mesh_component.hpp"
#include "components/mdl_skin_component.hpp"

#include "reflection/reflection.hpp"

bool mdlInit() {
    type_register<mdlModelMutable>("ModelMutable");
    type_register<mdlModelPrototype>("ModelPrototype")
        .prop("parents", &mdlModelPrototype::parents)
        .prop("name_to_node", &mdlModelPrototype::name_to_node)
        .prop("default_local_transforms", &mdlModelPrototype::default_local_transforms)
        .prop("components", &mdlModelPrototype::components);
    type_register<mdlModelInstance>("ModelInstance");

    mdlComponentRegister<mdlMeshComponent>("Mesh");
    mdlComponentRegister<mdlSkinComponent>("Skin");

    return true;
}
void mdlCleanup() {

}