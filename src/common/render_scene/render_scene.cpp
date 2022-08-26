#include "render_scene.hpp"


#include "render_object/scn_mesh_object.hpp"
#include "render_object/scn_skin.hpp"
#include "render_object/scn_decal.hpp"
#include "util/static_block.hpp"
STATIC_BLOCK{
    scnMeshObject::reflect();
    scnSkin::reflect();
    scnDecal::reflect();
}


scnRenderScene::scnRenderScene() {

}