#include "game/game_common.hpp"

void GameCommon::Update(float dt) {
    gfxm::vec3 loco_vec = inputCharaTranslation->getVec3();
    gfxm::mat3 loco_rot;
    loco_rot[2] = gfxm::normalize(cam.getView() * gfxm::vec4(0, 0, 1, 0));
    loco_rot[1] = gfxm::vec3(0, 1, 0);
    loco_rot[0] = gfxm::cross(loco_rot[1], loco_rot[2]);
    loco_vec = loco_rot * loco_vec;
    loco_vec.y = .0f;
    loco_vec = gfxm::normalize(loco_vec);

    chara.setDesiredLocomotionVector(loco_vec);
    if (inputCharaUse->isJustPressed()) {
        chara.actionUse();
    }
    chara.update(dt);

    world.update(dt);
}